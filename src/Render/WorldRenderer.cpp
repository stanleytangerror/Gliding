#include "RenderPch.h"
#include "WorldRenderer.h"
#include "RenderModule.h"
#include "D3D12Backend/D3D12Device.h"
#include "D3D12Backend/D3D12PipelinePass.h"
#include "D3D12Backend/D3D12RenderTarget.h"
#include "D3D12Backend/D3D12ResourceView.h"
#include "D3D12Backend/D3D12Resource.h"
#include "D3D12Backend/D3D12Geometry.h"
#include "D3D12Backend/D3D12Texture.h"
#include "World/Scene.h"
#include "RenderMaterial.h"
#include "RenderUtils.h"
#include "Light.h"
#include "EnvironmentMap.h"

WorldRenderer::WorldRenderer(RenderModule* renderModule, const Vec2i& renderSize)
	: mRenderModule(renderModule)
	, mRenderSize(renderSize)
{
	D3D12Device* device = mRenderModule->GetDevice();

	mSphere = D3D12Geometry::GenerateSphere(device, 20, 40);
	mQuad = D3D12Geometry::GenerateQuad(device);
	
	mSkyTexture = new D3D12Texture(device, R"(D:\Assets\Panorama_of_Marienplatz.dds)");
	mPanoramicSkySampler = new D3D12SamplerView(device, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	mLightingSceneSampler = new D3D12SamplerView(device, D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	mNoMipMapLinearSampler = new D3D12SamplerView(device, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	mFilteredEnvMapSampler = new D3D12SamplerView(device, D3D12_FILTER_MIN_MAG_MIP_LINEAR, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP });
	
	mBRDFIntegrationMapSampler = new D3D12SamplerView(device, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, { D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP });

	{
		D3D12_SAMPLER_DESC samplerDesc = {};
		{
			samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			std::fill_n(samplerDesc.BorderColor, Utils::GetArrayLength(samplerDesc.BorderColor), mSunLight->mLightViewProj.GetFarPlaneDeviceDepth());
			samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		}
		mNoMipMapLinearDepthCmpSampler = new D3D12SamplerView(device, samplerDesc);
	}

	for (i32 i = 0; i < mGBufferRts.size(); ++i)
	{
		mGBufferRts[i] = new D3D12RenderTarget(device, { mRenderSize.x(), mRenderSize.y(), 1 }, DXGI_FORMAT_R16G16B16A16_UNORM, Utils::FormatString("GBuffer%d", i).c_str());
	}
	mMainDepthRt = new D3DDepthStencil(device, mRenderSize,
		DXGI_FORMAT_R32G8X24_TYPELESS,
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
		DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
		"SceneDepthRt");

	mShadowMask = new D3D12RenderTarget(device, { mRenderSize.x(), mRenderSize.y(), 1 }, DXGI_FORMAT_R16_FLOAT, "ShadowMask");

	mSunLight = new DirectionalLight;
	{
		mSunLight->mLightIntensity = 1000.f;
		mSunLight->mWorldTransform.AlignCamera(
			Vec3f{ 1.f, 0.f, -1.f }.normalized(),
			Vec3f{ 1.f, 0.f, 1.f }.normalized(),
			Vec3f{ 0.f, -1.f, 0.f }.normalized());
		mSunLight->mWorldTransform.MoveCamera({ -200.f, 0.f, 200.f });
		mSunLight->mLightViewProj.mViewHeight = 200.f;
		mSunLight->mLightViewProj.mViewWidth = 200.f;
	}

	mLightViewDepthRt = new D3DDepthStencil(device, { 512, 512 },
		DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		"LightViewDepthRt");
	
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\monobike_derivative\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\seamless_pbr_texture_metal_01\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\free_1975_porsche_911_930_turbo\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\slum_house\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\city_test\scene.gltf)", Math::Axis3D_Yp);

	//mTestModel = RenderUtils::FromSceneRawData(device, sceneRawData);
	mTestModel = RenderUtils::GenerateMaterialProbes(device);

	mTestModel->mRelTransform = UniScalingf(10.f);
	//mTestModel->mRelTransform = Transformf(UniScalingf(25.f)) * Translationf(0.f, 0.f, -1.f);
	//mTestModel->mRelTransform = Translationf(0.f, 0.f, 10.f);
	//mTestModel->mRelTransform = Transformf(Translationf(0.f, 0.f, 100.f)) * Transformf(UniScalingf(0.01f));

	mCameraTrans.MoveCamera(200.f * Math::Axis3DDir<f32>(Math::Axis3D_Yn));
	mCameraProj.mFovHorizontal = Math::DegreeToRadian(90.f);
	mCameraProj.mAspectRatio = f32(renderSize.x()) / renderSize.y();
	mCameraProj.mFar = 100000.f;
}

WorldRenderer::~WorldRenderer()
{
	Utils::SafeDelete(mTestModel);
	Utils::SafeDelete(mQuad);
	Utils::SafeDelete(mSphere);
	Utils::SafeDelete(mSkyTexture);
	Utils::SafeDelete(mPanoramicSkyRt);
	Utils::SafeDelete(mMainDepthRt);
	for (auto& rt : mGBufferRts)
	{
		Utils::SafeDelete(rt);
	}
}

void WorldRenderer::TickFrame(Timer* timer)
{
	const f32 rad = Math::DegreeToRadian(10.f * timer->GetCurrentFrameElapsedSeconds());
	const Vec3f& camDir = Vec3f{ std::sin(rad), std::cos(rad), 0.f };
	const Vec3f& camUp = Math::Axis3DDir<f32>(Math::Axis3D_Zp);
	const Vec3f& camRight = camDir.cross(camUp);

	mCameraTrans.AlignCamera(camDir, camUp, camRight);
	mCameraTrans.MoveCamera(-100.f * camDir);

	//mTestModel->mRelTransform =
	//	Transformf(Math::FromAngleAxis(Math::DegreeToRadian(30.f * timer->GetLastFrameDeltaTime()), Math::Axis3DDir<f32>(Math::Axis3D_Zp)))
	//	* mTestModel->mRelTransform;
	mTestModel->CalcAbsTransform();
}

void WorldRenderer::Render(GraphicsContext* context, IRenderTargetView* target)
{
	{
		RENDER_EVENT(context, UpdateResources);

		if (!mSkyTexture->IsD3DResourceReady())
		{
			mSkyTexture->Initial(context);

			const auto& srcSize = mSkyTexture->GetSize();
			const Vec3i skyRtSize = { 1024, 1024 * srcSize.y() / srcSize.x(), 1 };
			mPanoramicSkyRt = new D3D12RenderTarget(context->GetDevice(), skyRtSize, DXGI_FORMAT_R32G32B32A32_FLOAT, "PanoramicSkyRt");

			const std::string& customSkyColor = Utils::FormatString("float4(color.xyz * %.2f, 1)", mSkyLightIntensity);
			RenderUtils::CopyTexture(context, mPanoramicSkyRt->GetRtv(), Vec2f::Zero(), Vec2f{ skyRtSize.x(), skyRtSize.y() }, mSkyTexture->GetSrv(), mNoMipMapLinearSampler, customSkyColor.c_str());

			mIrradianceMap = EnvironmentMap::GenerateIrradianceMap(context, mPanoramicSkyRt->GetSrv(), 8, 10);
			mFilteredEnvMap = EnvironmentMap::GeneratePrefilteredEnvironmentMap(context, mPanoramicSkyRt->GetSrv(), 1024);
		
			RenderUtils::GaussianBlur(context, mPanoramicSkyRt->GetRtv(), mPanoramicSkyRt->GetSrv(), 2);
		}

		if (!mBRDFIntegrationMap)
		{
			mBRDFIntegrationMap = EnvironmentMap::GenerateIntegratedBRDF(context, 1024);
		}

		mTestModel->ForEach([&](auto& node)
			{
				if (auto& mat = node.mContent.second)
				{
					mat->UpdateGpuResources(context);
				}
			});
	}

	//////////////////////////////////////////////////////////////////////////

	{
		RENDER_EVENT(context, LightViewDepth);

		const Vec3i& rtSize = target->GetResource()->GetSize();

		mLightViewDepthRt->Clear(context, mSunLight->mLightViewProj.GetFarPlaneDeviceDepth(), 0);

		mTestModel->ForEach([&](const auto& node)
			{
				D3D12Geometry* geo = node.mContent.first.get();
				RenderMaterial* mat = node.mContent.second.get();

				if (geo && mat && mat->IsGpuResourceReady())
				{
					RenderGeometryDepthWithMaterial(context, geo, mat, node.mAbsTransform, mSunLight->mWorldTransform, mSunLight->mLightViewProj, mLightViewDepthRt->GetDsv());
				}
			});
	}

	//////////////////////////////////////////////////////////////////////////

	{
		RENDER_EVENT(context, GBuffer);

		for (const auto& rt : mGBufferRts)
		{
			rt->Clear(context, { 0.f, 0.f, 0.f, 1.f });
		}
		mMainDepthRt->Clear(context, mCameraProj.GetFarPlaneDeviceDepth(), 0);

		mTestModel->ForEach([&](const auto& node)
			{
				D3D12Geometry* geo = node.mContent.first.get();
				RenderMaterial* mat = node.mContent.second.get();

				if (geo && mat && mat->IsGpuResourceReady())
				{
					RenderGeometryWithMaterial(context, geo, mat, node.mAbsTransform, mCameraTrans, mCameraProj, mGBufferRts, mMainDepthRt->GetDsv());
				}
			});
	}

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

void WorldRenderer::RenderShadowMaskChannel(GraphicsContext* context, IRenderTargetView* target)
{
	const Vec3i& targetSize = target->GetResource()->GetSize();
	const f32 width = f32(targetSize.x()) * 0.25f;
	const f32 height = f32(targetSize.y()) * 0.25f;;

	RenderUtils::CopyTexture(context,
		target, { 0.f, targetSize.y() - height }, { width, height },
		mShadowMask->GetSrv(), mNoMipMapLinearSampler, "float4(LinearToSrgb(color.xxx), 1)");
}

void WorldRenderer::RenderLightViewDepthChannel(GraphicsContext* context, IRenderTargetView* target)
{
	const Vec3i& targetSize = target->GetResource()->GetSize();
	const f32 size = f32(targetSize.y()) * 0.25f;

	RenderUtils::CopyTexture(context,
		target, { 0.f, size }, { size, size },
		mLightViewDepthRt->GetSrv(), mNoMipMapLinearSampler, "float4(LinearToSrgb(pow(color.xxx, 5)), 1)");
}

void WorldRenderer::DeferredLighting(GraphicsContext* context, IRenderTargetView* target)
{
	RENDER_EVENT(context, DeferredLighting);

	const auto& dsSize = mMainDepthRt->GetSize();
	std::unique_ptr<D3DDepthStencil> tmpDepth = std::make_unique<D3DDepthStencil>(context->GetDevice(), 
		Vec2i{ dsSize.x(), dsSize.y() },
		mMainDepthRt->GetFormat(),
		mMainDepthRt->GetDsv()->GetFormat(),
		mMainDepthRt->GetSrv()->GetFormat(),
		"TempDepthRt");
	context->CopyResource(tmpDepth.get(), mMainDepthRt);

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
		desc.DepthStencilState.StencilEnable = true;
		desc.DepthStencilState.StencilReadMask = RenderUtils::WorldStencilMask_OpaqueObject;
		desc.DepthStencilState.StencilWriteMask = 0;
		desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.InputLayout = { mQuad->mInputDescs.data(), u32(mQuad->mInputDescs.size()) };
	}

	const Vec3i& targetSize = target->GetResource()->GetSize();
	lightingPass.mRts[0] = target;
	lightingPass.mDs = tmpDepth->GetDsv();
	lightingPass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	lightingPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	lightingPass.mStencilRef = RenderUtils::WorldStencilMask_OpaqueObject;

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

	lightingPass.AddSrv("PrefilteredEnvMap", mFilteredEnvMap->GetSrv());
	lightingPass.AddSampler("PrefilteredEnvMapSampler", mFilteredEnvMapSampler);
	lightingPass.AddCbVar("PrefilteredInfo", Vec4f{ f32(mFilteredEnvMap->GetMipLevelCount()), 0.f, 0.f, 0.f });
	
	lightingPass.AddSrv("IrradianceMap", mIrradianceMap->GetSrv());
	lightingPass.AddSampler("IrradianceMapSampler", mPanoramicSkySampler);

	lightingPass.AddSrv("BRDFIntegrationMap", mBRDFIntegrationMap->GetSrv());
	lightingPass.AddSampler("BRDFIntegrationMapSampler", mBRDFIntegrationMapSampler);

	lightingPass.AddCbVar("CameraDir", mCameraTrans.CamDirInWorldSpace());
	lightingPass.AddCbVar("CameraPos", mCameraTrans.CamPosInWorldSpace()); 
	lightingPass.AddCbVar("LightDir", mSunLight->mWorldTransform.CamDirInWorldSpace());
	lightingPass.AddCbVar("LightColor", (mSunLight->mLightColor * mSunLight->mLightIntensity).eval());

	lightingPass.Draw();
}

void WorldRenderer::RenderSky(GraphicsContext* context, IRenderTargetView* target, DSV* depth) const
{
	if (!mPanoramicSkyRt) { return; }

	RENDER_EVENT(context, Sky);

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

	pass.AddSrv("PanoramicSky", mPanoramicSkyRt->GetSrv());
	pass.AddSampler("PanoramicSkySampler", mPanoramicSkySampler);

	pass.Draw();
}

void WorldRenderer::RenderGeometryWithMaterial(GraphicsContext* context, 
	D3D12Geometry* geometry, RenderMaterial* material, 
	const Transformf& transform, 
	const Math::CameraTransformf& cameraTrans, const Math::PerspectiveProjectionf& cameraProj, 
	const std::array<D3D12RenderTarget*, 3>& gbufferRts, DSV* depthView)
{
	PROFILE_EVENT(WorldRenderer::RenderGeometryWithMaterial);
	
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

	const std::pair<MaterialParamSemantic, std::string> semanticSlots[] =
	{
		{ TextureUsage_Normal,			 "Normal" },
		{ TextureUsage_Metalness,		 "Metallic" },
		{ TextureUsage_BaseColor,		 "BaseColor" },
		{ TextureUsage_Roughness,		 "Roughness" },
	};

	for (const auto& [usage, paramName] : semanticSlots)
	{
		const auto& attr = material->mMatAttriSlots[usage];
		if (attr.mTexture && attr.mTexture->IsD3DResourceReady())
		{
			gbufferPass.mShaderMacros.push_back(ShaderMacro{ paramName + "_USE_MAP", "" });

			gbufferPass.AddSrv((paramName + "Tex").c_str(), attr.mTexture->GetSrv());
			gbufferPass.AddSampler((paramName + "Sampler").c_str(), attr.mSampler);
		}
		else
		{
			gbufferPass.AddCbVar((paramName + "ConstantValue").c_str(), attr.mConstantValue);
		}
	}

	gbufferPass.Draw();
}

void WorldRenderer::RenderGeometryDepthWithMaterial(GraphicsContext* context, D3D12Geometry* geometry, RenderMaterial* material, const Transformf& transform, const Math::CameraTransformf& cameraTrans, const Math::OrthographicProjectionf& cameraProj, DSV* depthView)
{
	PROFILE_EVENT(WorldRenderer::RenderGeometryDepthWithMaterial);

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

	const auto& attr = material->mMatAttriSlots[TextureUsage_BaseColor];
	if (attr.mTexture && attr.mTexture->IsD3DResourceReady())
	{
		pass.AddSrv(paramName, attr.mTexture->GetSrv());
		pass.AddSampler((std::string(paramName) + "Sampler").c_str(), attr.mSampler);
	}

	pass.Draw();
}

void WorldRenderer::RenderShadowMask(GraphicsContext* context, 
	IRenderTargetView* shadowMask, 
	IShaderResourceView* lightViewDepth, D3D12SamplerView* lightViewDepthSampler,
	IShaderResourceView* cameraViewDepth, D3D12SamplerView* cameraViewDepthSampler,
	const Math::OrthographicProjectionf& lightViewProj, const Math::CameraTransformf& lightViewTrans,
	const Math::PerspectiveProjectionf& cameraProj, const Math::CameraTransformf& cameraTrans)
{
	RENDER_EVENT(context, ShadowMask);

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
