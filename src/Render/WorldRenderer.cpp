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

	const auto& size = renderModule->GetBackBufferSize();
	for (i32 i = 0; i < mGBufferRts.size(); ++i)
	{
		mGBufferRts[i] = new D3D12RenderTarget(device, { size.x(), size.y(), 1 }, DXGI_FORMAT_R16G16B16A16_UNORM, Utils::FormatString("GBuffer%d", i).c_str());
	}
	mDepthRt = new D3DDepthStencil(device, size,
		DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		"SceneDepthRt");

	SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\monobike_derivative\scene.gltf)", Math::Axis3D_Yp);
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

	//mTestModel.mRelTransform = UniScalingf(40.f);

	mLight.mLightColor = Vec3f::Ones() * 1000.f;
	mLight.mLightDir = Vec3f(0.f, 1.f, -1.f).normalized();

	mCameraTrans.mWorldTransform = Translationf(200.f * Math::Axis3DDir<f32>(Math::Axis3D_Yn));
}

WorldRenderer::~WorldRenderer()
{
	Utils::SafeDelete(mQuad);
	Utils::SafeDelete(mSphere);
	Utils::SafeDelete(mPanoramicSkyTex);
	Utils::SafeDelete(mDepthRt);
	for (auto& rt : mGBufferRts)
	{
		Utils::SafeDelete(rt);
	}
}

void WorldRenderer::TickFrame(Timer* timer)
{
	mCameraTrans.mWorldTransform = 
		Transformf(Rotationf(Math::DegreeToRadian(10.f * timer->GetLastFrameDeltaTime()), Math::Axis3DDir<f32>(Math::Axis3D_Zp)))
		* mCameraTrans.mWorldTransform;

	mTestModel.mRelTransform =
		Transformf(Rotationf(Math::DegreeToRadian(90.f * timer->GetLastFrameDeltaTime()), Math::Axis3DDir<f32>(Math::Axis3D_Zp)))
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
	
	{
		for (const auto& rt : mGBufferRts)
		{
			rt->Clear(context, { 0.f, 0.f, 0.f, 1.f });
		}
		mDepthRt->Clear(context, mCameraProj.GetFarPlaneDepth(), 0);
	}

	mTestModel.ForEach([&](const auto& node)
		{
			D3D12Geometry* geo = node.mContent.first.get();
			RenderMaterial* mat = node.mContent.second.get();

			if (geo && mat && mat->IsGpuResourceReady())
			{
				RenderGeometryWithMaterial(context, geo, mat, node.mAbsTransform);
			}
		});

	//////////////////////////////////////////////////////////////////////////

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
		lightingPass.AddSrv("GBuffer0", mGBufferRts[0]->GetSrv());
		lightingPass.AddSrv("GBuffer1", mGBufferRts[1]->GetSrv());
		lightingPass.AddSrv("GBuffer2", mGBufferRts[2]->GetSrv());

		lightingPass.AddSrv("PanoramicSky", mPanoramicSkyTex->GetSrv());

		lightingPass.AddCbVar("CameraDir", mCameraTrans.CamDirInWorldSpace());
		lightingPass.AddCbVar("LightDir", mLight.mLightDir);
		lightingPass.AddCbVar("LightColor", mLight.mLightColor);

		lightingPass.Draw();
	}

	RenderSky(context, target, mDepthRt->GetDsv());
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
		desc.DepthStencilState.StencilReadMask = mSceneMask & (~mSkyMask);
		desc.DepthStencilState.StencilWriteMask = mSkyMask;
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
	pass.AddCbVar("FrustumInfo", Vec4f{ mCameraProj.mAspectRatio, mCameraProj.GetFovVertical(), mCameraProj.GetFovHorizontal(), 0.f });
	pass.AddCbVar("CameraDir", mCameraTrans.CamDirInWorldSpace());
	pass.AddCbVar("InvViewMat", mCameraTrans.ComputeInvViewMatrix());

	pass.AddSrv("PanoramicSky", mPanoramicSkyTex->GetSrv());
	pass.AddSampler("PanoramicSkySampler", mPanoramicSkySampler);

	pass.Draw();
}

void WorldRenderer::RenderGeometryWithMaterial(GraphicsContext* context, D3D12Geometry* geometry, RenderMaterial* material, const Transformf& transform) const
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
		desc.DepthStencilState.DepthFunc = D3D12Utils::ToDepthCompareFunc(mCameraProj.GetNearerDepthCompare());
		desc.DepthStencilState.StencilEnable = true;
		desc.DepthStencilState.StencilReadMask = mSceneMask;
		desc.DepthStencilState.StencilWriteMask = mOpaqueObjMask;
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

	for (i32 i = 0; i < mGBufferRts.size(); ++i)
	{
		gbufferPass.mRts[i] = mGBufferRts[i]->GetRtv();
	}
	gbufferPass.mDs = mDepthRt->GetDsv();

	const Vec3i& targetSize = mGBufferRts[0]->GetSize();
	gbufferPass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	gbufferPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	gbufferPass.mStencilRef = mOpaqueObjMask;

	gbufferPass.mVbvs.clear();
	gbufferPass.mVbvs.push_back(geometry->mVbv);
	gbufferPass.mIbv = geometry->mIbv;
	gbufferPass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	gbufferPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

	gbufferPass.AddCbVar("worldMat", transform.matrix());
	gbufferPass.AddCbVar("viewMat", mCameraTrans.ComputeViewMatrix());
	gbufferPass.AddCbVar("projMat", mCameraProj.ComputeProjectionMatrix());

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

