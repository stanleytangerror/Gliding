#include "RenderPch.h"
#include "WorldRenderer.h"
#include "RenderModule.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12PipelinePass.h"
#include "D3D12/D3D12RenderTarget.h"
#include "D3D12/D3D12ResourceView.h"
#include "D3D12/D3D12Resource.h"
#include "D3D12/D3D12Geometry.h"
#include "D3D12/D3D12Texture.h"
#include "World/Scene.h"
#include "RenderMaterial.h"
#include "RenderUtils.h"
#include "Light.h"

namespace
{
	D3D12_SAMPLER_DESC ToD3DSamplerDesc(const TextureSamplerType& type)
	{
		D3D12_SAMPLER_DESC result = {};

		auto mapType = [](SamplerAddrMode mode)
		{
			switch (mode)
			{
			case TextureSamplerType_Clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			case TextureSamplerType_Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			case TextureSamplerType_Wrap: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			case TextureSamplerType_Decal:
			default:  Assert(false); return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			}
		};

		result.AddressU = mapType(type[0]);
		result.AddressV = mapType(type[1]);
		result.AddressW = mapType(type[2]);

		return result;
	}
}

WorldRenderer::WorldRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	D3D12Device* device = mRenderModule->GetDevice();

	mSphere = D3D12Geometry::GenerateSphere(device, 20, 40);
	mQuad = D3D12Geometry::GenerateQuad(device);
	
	mPanoramicSkyTex = new D3D12Texture(device, R"(D:\Assets\Panorama_of_Marienplatz.dds)");
	mPanoramicSkySampler = new D3D12SamplerView(device, D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	mLightingSceneSampler = new D3D12SamplerView(device, D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	mNoMipMapLinearSampler = new D3D12SamplerView(device, D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });

	{
		D3D12_SAMPLER_DESC samplerDesc = {};
		{
			samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		}
		mNoMipMapLinearDepthCmpSampler = new D3D12SamplerView(device, samplerDesc);
	}

	const auto& size = renderModule->GetBackBufferSize();
	for (i32 i = 0; i < mGBufferRts.size(); ++i)
	{
		mGBufferRts[i] = new D3D12RenderTarget(device, { size.x(), size.y(), 1 }, DXGI_FORMAT_R16G16B16A16_UNORM, Utils::FormatString("GBuffer%d", i).c_str());
	}
	mMainDepthRt = new D3DDepthStencil(device, size,
		DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		"SceneDepthRt");

	mShadowMask = new D3D12RenderTarget(device, { size.x(), size.y(), 1 }, DXGI_FORMAT_R16_FLOAT, "ShadowMask");

	mSunLight = new DirectionalLight;
	{
		mSunLight->mLightIntensity = 1000.f;
		mSunLight->mWorldTransform.AlignCamera(
			Vec3f{ 1.f, 0.f, -1.f }.normalized(),
			Vec3f{ 1.f, 0.f, 1.f }.normalized(),
			Vec3f{ 0.f, -1.f, 0.f }.normalized());
		mSunLight->mWorldTransform.MoveCamera({ -200.f, 0.f, 200.f });
		mSunLight->mLightViewProj.mViewHeight = 600.f;
		mSunLight->mLightViewProj.mViewWidth = 600.f;
	}

	mLightViewDepthRt = new D3DDepthStencil(device, { 512, 512 },
		DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		"LightViewDepthRt");

	SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\monobike_derivative\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\seamless_pbr_texture_metal_01\scene.gltf)", Math::Axis3D_Yp);
	std::map<std::string, std::pair<D3D12Texture*, D3D12SamplerView*>> textures;
	for (const auto& p : sceneRawData->mTextures)
	{
		const std::string& texPath = p.first;
		const TextureRawData* texRawData = p.second;

		if (texRawData)
		{
			textures[texPath].first = new D3D12Texture(device, texPath.c_str(), texRawData->mRawData);
		}
		textures[texPath].second = new D3D12SamplerView(device, ToD3DSamplerDesc(texRawData->mSamplerType));
	}
	std::vector<std::shared_ptr<RenderMaterial>> materials;
	for (const auto& mat : sceneRawData->mMaterials)
	{
		materials.emplace_back(RenderMaterial::GenerateRenderMaterialFromRawData(mat, textures));
	}
	for (MeshRawData* mesh : sceneRawData->mMeshes)
	{
		D3D12Geometry* geo = D3D12Geometry::GenerateGeometryFromMeshRawData(device, mesh);
		const auto& mat = materials[mesh->mMaterialIndex];
		const Transformf& trans = mesh->mTransform;

		mTestModel.PushChild({
			std::unique_ptr<D3D12Geometry>(geo),
			mat },
			trans);
	}

	//mTestModel.mRelTransform = Transformf(UniScalingf(25.f)) * Translationf(0.f, 0.f, -0.7f);
	mTestModel.mRelTransform = Translationf(0.f, 0.f, 10.f);

	mCameraTrans.MoveCamera(200.f * Math::Axis3DDir<f32>(Math::Axis3D_Yn));
}

WorldRenderer::~WorldRenderer()
{
	Utils::SafeDelete(mQuad);
	Utils::SafeDelete(mSphere);
	Utils::SafeDelete(mPanoramicSkyTex);
	Utils::SafeDelete(mMainDepthRt);
	for (auto& rt : mGBufferRts)
	{
		Utils::SafeDelete(rt);
	}
}

void WorldRenderer::TickFrame(Timer* timer)
{
	//mCameraTrans.mWorldTransform = 
	//	Transformf(Rotationf(Math::DegreeToRadian(30.f * timer->GetLastFrameDeltaTime()), Math::Axis3DDir<f32>(Math::Axis3D_Zp)))
	//	* mCameraTrans.mWorldTransform;

	mTestModel.mRelTransform =
		Transformf(Math::FromAngleAxis(Math::DegreeToRadian(90.f * timer->GetLastFrameDeltaTime()), Math::Axis3DDir<f32>(Math::Axis3D_Zp)))
		* mTestModel.mRelTransform;
	mTestModel.CalcAbsTransform();
}

void WorldRenderer::Render(GraphicsContext* context, IRenderTargetView* target)
{
	if (!mPanoramicSkyTex->IsD3DResourceReady())
	{
		mPanoramicSkyTex->Initial(context);
	}

	mTestModel.ForEach([&](auto& node)
		{
			if (auto& mat = node.mContent.second)
			{
				mat->UpdateGpuResources(context);
			}
		});

	//////////////////////////////////////////////////////////////////////////

	const Vec3i& rtSize = target->GetResource()->GetSize();

	mLightViewDepthRt->Clear(context, mSunLight->mLightViewProj.GetFarPlaneDeviceDepth(), 0);

	mTestModel.ForEach([&](const auto& node)
		{
			D3D12Geometry* geo = node.mContent.first.get();
			RenderMaterial* mat = node.mContent.second.get();

			if (geo && mat && mat->IsGpuResourceReady())
			{
				RenderGeometryDepthWithMaterial(context, geo, mat, node.mAbsTransform, mSunLight->mWorldTransform, mSunLight->mLightViewProj, mLightViewDepthRt->GetDsv());
			}
		});

	//////////////////////////////////////////////////////////////////////////

	for (const auto& rt : mGBufferRts)
	{
		rt->Clear(context, { 0.f, 0.f, 0.f, 1.f });
	}
	mMainDepthRt->Clear(context, mCameraProj.GetFarPlaneDeviceDepth(), 0);

	mTestModel.ForEach([&](const auto& node)
		{
			D3D12Geometry* geo = node.mContent.first.get();
			RenderMaterial* mat = node.mContent.second.get();

			if (geo && mat && mat->IsGpuResourceReady())
			{
				RenderGeometryWithMaterial(context, geo, mat, node.mAbsTransform, mCameraTrans, mCameraProj, mGBufferRts, mMainDepthRt->GetDsv());
			}
		});

	//////////////////////////////////////////////////////////////////////////

	RenderShadowMask(context, mShadowMask->GetRtv(), mLightViewDepthRt->GetSrv(), mNoMipMapLinearDepthCmpSampler, mMainDepthRt->GetSrv(), mNoMipMapLinearSampler,
		mSunLight->mLightViewProj, mSunLight->mWorldTransform, mCameraProj, mCameraTrans);
	DeferredLighting(context, target);
	RenderSky(context, target, mMainDepthRt->GetDsv());
}

void WorldRenderer::RenderGBufferChannels(GraphicsContext* context, IRenderTargetView* target)
{
	const std::pair<i32, const char*> gbufferSemantics[] =
	{
		{ 1, "float4(LinearToSrgb(color.xyz), 1)" },		// BaseColor,
		{ 0, "float4(LinearToSrgb(color.xyz), 1)" },		// Normal,
		{ 2, "float4(LinearToSrgb(color.yyy), 1)" },		// MetalMash,
		{ 0, "float4(LinearToSrgb(1.0 - color.www), 1)" },	// Roughness,
		{ 2, "float4(LinearToSrgb(color.zzz), 1)" },		// Reflection,
	};
	
	const Vec3i& targetSize = target->GetResource()->GetSize();
	const f32 width = f32(targetSize.x()) / Utils::GetArrayLength(gbufferSemantics);
	const f32 height = width / targetSize.x() * targetSize.y();

	for (u32 i = 0; i < Utils::GetArrayLength(gbufferSemantics); ++i)
	{
		const auto& [idx, unary] = gbufferSemantics[i];

		RenderUtils::CopyTexture(context, 
			target, { i * width, 0.f }, { width, height }, 
			mGBufferRts[idx]->GetSrv(), mNoMipMapLinearSampler, unary);
	}
}

void WorldRenderer::RenderShadowMaskChannels(GraphicsContext* context, IRenderTargetView* target)
{
	const Vec3i& targetSize = target->GetResource()->GetSize();
	const f32 width = f32(targetSize.x()) * 0.25f;
	const f32 height = f32(targetSize.y()) * 0.25f;;

	RenderUtils::CopyTexture(context,
		target, { 0.f, targetSize.y() - height }, { width, height },
		mShadowMask->GetSrv(), mNoMipMapLinearSampler, "float4(LinearToSrgb(color.xxx), 1)");
}

void WorldRenderer::DeferredLighting(GraphicsContext* context, IRenderTargetView* target)
{
	GraphicsPass lightingPass(context);

	lightingPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	lightingPass.mRootSignatureDesc.mEntry = "GraphicsRS";
	lightingPass.mVsFile = "res/Shader/Lighting.hlsl";
	lightingPass.mPsFile = "res/Shader/Lighting.hlsl";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = lightingPass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { mQuad->mInputDescs.data(), u32(mQuad->mInputDescs.size()) };
	}

	const Vec3i& targetSize = target->GetResource()->GetSize();
	lightingPass.mRts[0] = target;
	lightingPass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	lightingPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

	lightingPass.mVbvs.clear();
	lightingPass.mVbvs.push_back(mQuad->mVbv);
	lightingPass.mIbv = mQuad->mIbv;
	lightingPass.mIndexCount = mQuad->mIbv.SizeInBytes / sizeof(u16);

	lightingPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
	lightingPass.AddCbVar("FrustumInfo", Vec4f{ mCameraProj.GetHalfFovHorizontal(), mCameraProj.GetHalfFovVertical(), mCameraProj.mNear, mCameraProj.mFar });
	lightingPass.AddSrv("GBuffer0", mGBufferRts[0]->GetSrv());
	lightingPass.AddSrv("GBuffer1", mGBufferRts[1]->GetSrv());
	lightingPass.AddSrv("GBuffer2", mGBufferRts[2]->GetSrv());
	lightingPass.AddSrv("ShadowMask", mShadowMask->GetSrv());
	lightingPass.AddSampler("ShadowMaskSampler", mLightingSceneSampler);
	lightingPass.AddSrv("SceneDepth", mMainDepthRt->GetSrv());
	lightingPass.AddSampler("GBufferSampler", mLightingSceneSampler);

	lightingPass.AddCbVar("InvViewMat", mCameraTrans.ComputeInvViewMatrix());
	lightingPass.AddCbVar("InvProjMat", mCameraProj.ComputeInvProjectionMatrix());

	lightingPass.AddSrv("PanoramicSky", mPanoramicSkyTex->GetSrv());
	lightingPass.AddSampler("PanoramicSkySampler", mPanoramicSkySampler);

	lightingPass.AddCbVar("CameraDir", mCameraTrans.CamDirInWorldSpace());
	lightingPass.AddCbVar("CameraPos", mCameraTrans.CamPosInWorldSpace()); 
	lightingPass.AddCbVar("LightDir", mSunLight->mWorldTransform.CamDirInWorldSpace());
	lightingPass.AddCbVar("LightColor", (mSunLight->mLightColor * mSunLight->mLightIntensity).eval());

	lightingPass.Draw();
}


void WorldRenderer::RenderSky(GraphicsContext* context, IRenderTargetView* target, DSV* depth) const
{
	GraphicsPass pass(context);

	D3D12Geometry* geometry = mQuad;
	const Transformf& transform = Transformf(UniScalingf(1000.f));

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/PanoramicSky.hlsl";
	pass.mPsFile = "res/Shader/PanoramicSky.hlsl";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = true;
		desc.DepthStencilState.StencilReadMask = RenderUtils::WorldStencilMask_Scene & (~RenderUtils::WorldStencilMask_Sky);
		desc.DepthStencilState.StencilWriteMask = RenderUtils::WorldStencilMask_Sky;
		desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		desc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc.InputLayout = { geometry->mInputDescs.data(), u32(geometry->mInputDescs.size()) };
	}

	pass.mRts[0] = target;
	pass.mDs = depth;
	const Vec3i& targetSize = target->GetResource()->GetSize();
	pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	pass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	pass.mStencilRef = 0;

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->mVbv);
	pass.mIbv = geometry->mIbv;
	pass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
	pass.AddCbVar("FrustumInfo", Vec4f{ mCameraProj.GetHalfFovHorizontal(), mCameraProj.GetHalfFovVertical(), mCameraProj.mNear, mCameraProj.mFar });
	pass.AddCbVar("CameraDir", mCameraTrans.CamDirInWorldSpace());
	pass.AddCbVar("InvViewMat", mCameraTrans.ComputeInvViewMatrix());

	pass.AddSrv("PanoramicSky", mPanoramicSkyTex->GetSrv());
	pass.AddSampler("PanoramicSkySampler", mPanoramicSkySampler);

	pass.Draw();
}

void WorldRenderer::RenderGeometryWithMaterial(GraphicsContext* context, 
	D3D12Geometry* geometry, RenderMaterial* material, 
	const Transformf& transform, 
	const Math::CameraTransformf& cameraTrans, const Math::PerspectiveProjection& cameraProj, 
	const std::array<D3D12RenderTarget*, 3>& gbufferRts, DSV* depthView)
{
	GraphicsPass gbufferPass(context);

	gbufferPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	gbufferPass.mRootSignatureDesc.mEntry = "GraphicsRS";
	gbufferPass.mVsFile = "res/Shader/GBufferPBRMat01.hlsl";
	gbufferPass.mPsFile = "res/Shader/GBufferPBRMat01.hlsl";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = gbufferPass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = true;
		desc.DepthStencilState.DepthFunc = D3D12Utils::ToDepthCompareFunc(cameraProj.GetNearerDepthCompare());
		desc.DepthStencilState.StencilEnable = true;
		desc.DepthStencilState.StencilReadMask = RenderUtils::WorldStencilMask_Scene;
		desc.DepthStencilState.StencilWriteMask = RenderUtils::WorldStencilMask_OpaqueObject;
		desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		desc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.InputLayout = { geometry->mInputDescs.data(), u32(geometry->mInputDescs.size()) };
	}

	for (i32 i = 0; i < gbufferRts.size(); ++i)
	{
		gbufferPass.mRts[i] = gbufferRts[i]->GetRtv();
	}
	gbufferPass.mDs = depthView;

	const Vec3i& targetSize = gbufferRts[0]->GetSize();
	gbufferPass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	gbufferPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	gbufferPass.mStencilRef = RenderUtils::WorldStencilMask_OpaqueObject;

	gbufferPass.mVbvs.clear();
	gbufferPass.mVbvs.push_back(geometry->mVbv);
	gbufferPass.mIbv = geometry->mIbv;
	gbufferPass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	gbufferPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

	gbufferPass.AddCbVar("worldMat", transform.matrix());
	gbufferPass.AddCbVar("viewMat", cameraTrans.ComputeViewMatrix());
	gbufferPass.AddCbVar("projMat", cameraProj.ComputeProjectionMatrix());

	const std::pair<TextureUsage, const char*> texSlots[] =
	{
		{ TextureUsage_Normal,			 "NormalTex" },
		{ TextureUsage_Metalness,		 "MetalnessTex" },
		{ TextureUsage_BaseColor,		 "BaseColorTex" },
		{ TextureUsage_DiffuseRoughness, "DiffuseRoughnessTex" },
	};

	for (const auto& p : texSlots)
	{
		const TextureUsage usage = p.first;
		const char* paramName = p.second;

		const auto& texs = material->mTextureParams[usage];
		if (!texs.empty())
		{
			gbufferPass.AddSrv(paramName, texs.front()->GetSrv());
			gbufferPass.AddSampler((std::string(paramName) + "Sampler").c_str(), material->mSamplerParams[usage].front());
		}
	}

	gbufferPass.Draw();
}

void WorldRenderer::RenderGeometryDepthWithMaterial(GraphicsContext* context, D3D12Geometry* geometry, RenderMaterial* material, const Transformf& transform, const Math::CameraTransformf& cameraTrans, const Math::OrthographicProjection& cameraProj, DSV* depthView)
{
	GraphicsPass pass(context);

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/GeometryDepth.hlsl";
	pass.mPsFile = "res/Shader/GeometryDepth.hlsl";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.RasterizerState.DepthBias = 10000;
		desc.RasterizerState.SlopeScaledDepthBias = 10;
		desc.DepthStencilState.DepthEnable = true;
		desc.DepthStencilState.DepthFunc = D3D12Utils::ToDepthCompareFunc(cameraProj.GetNearerDepthCompare());
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { geometry->mInputDescs.data(), u32(geometry->mInputDescs.size()) };
	}

	pass.mDs = depthView;

	const Vec3i& targetSize = depthView->GetResource()->GetSize();
	pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	pass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	pass.mStencilRef = RenderUtils::WorldStencilMask_OpaqueObject;

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->mVbv);
	pass.mIbv = geometry->mIbv;
	pass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

	pass.AddCbVar("worldMat", transform.matrix());
	pass.AddCbVar("viewMat", cameraTrans.ComputeViewMatrix());
	pass.AddCbVar("projMat", cameraProj.ComputeProjectionMatrix());

	const char* paramName = "BaseColorTex";

	const auto& texs = material->mTextureParams[TextureUsage_BaseColor];
	if (!texs.empty())
	{
		pass.AddSrv(paramName, texs.front()->GetSrv());
		pass.AddSampler((std::string(paramName) + "Sampler").c_str(), material->mSamplerParams[TextureUsage_BaseColor].front());
	}

	pass.Draw();
}

void WorldRenderer::RenderShadowMask(GraphicsContext* context, 
	IRenderTargetView* shadowMask, 
	IShaderResourceView* lightViewDepth, D3D12SamplerView* lightViewDepthSampler,
	IShaderResourceView* cameraViewDepth, D3D12SamplerView* cameraViewDepthSampler,
	const Math::OrthographicProjection& lightViewProj, const Math::CameraTransformf& lightViewTrans,
	const Math::PerspectiveProjection& cameraProj, const Math::CameraTransformf& cameraTrans)
{
	static D3D12Geometry* geometry = D3D12Geometry::GenerateQuad(context->GetDevice());

	GraphicsPass pass(context);

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/ConstructShadowMask.hlsl";
	pass.mPsFile = "res/Shader/ConstructShadowMask.hlsl";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.DepthFunc = D3D12Utils::ToDepthCompareFunc(cameraProj.GetNearerDepthCompare());
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { geometry->mInputDescs.data(), u32(geometry->mInputDescs.size()) };
	}

	pass.mRts[0] = shadowMask;

	const Vec3i& targetSize = shadowMask->GetResource()->GetSize();
	pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	pass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->mVbv);
	pass.mIbv = geometry->mIbv;
	pass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	pass.AddSrv("LightViewDepth", lightViewDepth);
	pass.AddSampler("LightViewDepthSampler", lightViewDepthSampler);
	pass.AddSrv("CameraViewDepth", cameraViewDepth);
	pass.AddSampler("CameraViewDepthSampler", cameraViewDepthSampler);

	pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
	pass.AddCbVar("FrustumInfo", Vec4f{ cameraProj.GetHalfFovHorizontal(), cameraProj.GetHalfFovVertical(), cameraProj.mNear, cameraProj.mFar });

	pass.AddCbVar("CameraViewMat", cameraTrans.ComputeViewMatrix());
	pass.AddCbVar("CameraInvViewMat", cameraTrans.ComputeInvViewMatrix());
	pass.AddCbVar("CameraProjMat", cameraProj.ComputeProjectionMatrix());
	pass.AddCbVar("CameraInvProjMat", cameraProj.ComputeInvProjectionMatrix());

	pass.AddCbVar("LightViewMat", lightViewTrans.ComputeViewMatrix());
	pass.AddCbVar("LightProjMat", lightViewProj.ComputeProjectionMatrix());

	pass.Draw();
}
