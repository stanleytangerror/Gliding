#include "RenderPch.h"
#include "WorldRenderer.h"
#include "RenderModule.h"
#include "Geometry.h"
#include "Texture.h"
#include "World/Scene.h"
#include "RenderMaterial.h"
#include "RenderUtils.h"
#include "Light.h"
#include "EnvironmentMap.h"
#include "FrameGraph.h"

struct LightViewData
{
	FrameGraphMutableResource	mLightViewDepth;
};

struct EnvLighting
{
	FrameGraphMutableResource	mPanoramicSky;
	FrameGraphResource			mFilteredEnvMap;
	FrameGraphResource			mIrradianceMap;
	FrameGraphResource			mBRDFIntegrationMap;
};

struct GBufferData
{
	std::array<FrameGraphMutableResource, 3> mGBuffers = {};
};

WorldRenderer::WorldRenderer(RenderModule* renderModule, const Vec2u& renderSize)
	: mRenderModule(renderModule)
	, mRenderSize(renderSize)
{
	auto infra = mRenderModule->GetGraphicsInfra();
	auto* frameGraph = renderModule->GetFrameGraph();
	auto* blackboard = frameGraph->GetBlackboard();

	blackboard->Add<LightViewData>();
	blackboard->Add<EnvLighting>();
	blackboard->Add<GBufferData>();

	auto& cam = blackboard->Add<MainCameraState>();
	{
		cam.mCameraTrans.MoveCamera(200.f * Math::Axis3DDir<f32>(Math::Axis3D_Yn));
		cam.mCameraProj.mFovHorizontal = Math::DegreeToRadian(90.f);
		cam.mCameraProj.mAspectRatio = f32(mRenderSize.x()) / mRenderSize.y();
		cam.mCameraProj.mFar = 100000.f;
	}

	auto& sunLight = blackboard->Add<DirectionalLight>();
	{
		sunLight.mLightIntensity = 1000.f;
		sunLight.mWorldTransform.AlignCamera(
			Vec3f{ 1.f, 0.f, -1.f }.normalized(),
			Vec3f{ 1.f, 0.f, 1.f }.normalized(),
			Vec3f{ 0.f, -1.f, 0.f }.normalized());
		sunLight.mWorldTransform.MoveCamera({ -200.f, 0.f, 200.f });
		sunLight.mLightViewProj.mViewHeight = 200.f;
		sunLight.mLightViewProj.mViewWidth = 200.f;
	}

	mSphere.reset(Geometry::GenerateSphere(40)->CreateAndInitialResource(frameGraph));
	mQuad.reset(Geometry::GenerateQuad()->CreateAndInitialResource(frameGraph));
	
	const char* skyTexPath = R"(D:\Assets\Panorama_of_Marienplatz.dds)";
	mSkyTexture = std::make_unique<FileTexture>(frameGraph, skyTexPath, Utils::LoadFileContent(skyTexPath));

	mPanoramicSkySampler
		.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
		.SetAddressXYZ(GI::TextureAddressMode::WRAP);

	mLightingSceneSampler
		.SetFilter(GI::Filter::MIN_MAG_POINT_MIP_LINEAR)
		.SetAddressXYZ(GI::TextureAddressMode::WRAP);
	mNoMipMapLinearSampler
		.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
		.SetAddressXYZ(GI::TextureAddressMode::WRAP);
	mFilteredEnvMapSampler
		.SetFilter(GI::Filter::MIN_MAG_MIP_LINEAR)
		.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::CLAMP, GI::TextureAddressMode::CLAMP });

	mBRDFIntegrationMapSampler
		.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
		.SetAddressXYZ(GI::TextureAddressMode::CLAMP);

	const f32 farPlaneDeviceDepth = sunLight.mLightViewProj.GetFarPlaneDeviceDepth();
		
	mNoMipMapLinearDepthCmpSampler
		.SetFilter(GI::Filter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT)
		.SetAddressXYZ(GI::TextureAddressMode::BORDER)
		.SetBorderColor(Vec4f::Ones() * farPlaneDeviceDepth)
		.SetComparisonFunc(GI::ComparisonFunction::LESS_EQUAL);

	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\monobike_derivative\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\seamless_pbr_texture_metal_01\scene.gltf)", Math::Axis3D_Yp);
	SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\free_1975_porsche_911_930_turbo\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\slum_house\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\city_test\scene.gltf)", Math::Axis3D_Yp);

	mTestModel.reset(RenderUtils::FromSceneRawData(frameGraph, sceneRawData));
	//mTestModel.reset(RenderUtils::GenerateMaterialProbes(device));

	//mTestModel->mRelTransform = UniScalingf(10.f);
	mTestModel->mRelTransform = Transformf(UniScalingf(25.f)) * Translationf(0.f, 0.f, -1.f);
	//mTestModel->mRelTransform = Translationf(0.f, 0.f, 10.f);
	//mTestModel->mRelTransform = Transformf(Translationf(0.f, 0.f, 100.f)) * Transformf(UniScalingf(0.01f));
}

WorldRenderer::~WorldRenderer()
{

}

void WorldRenderer::TickFrame(Timer* timer)
{
	mTestModel->CalcAbsTransform();
}

FrameGraphMutableResource WorldRenderer::Render()
{
	auto* frameGraph = mRenderModule->GetFrameGraph();
	auto* blackboard = frameGraph->GetBlackboard();

	auto& sunLight = blackboard->Get<DirectionalLight>();
	auto& lightView = blackboard->Get<LightViewData>();
	auto& cameraView = blackboard->Get<MainCameraState>();
	auto& envLighting = blackboard->Get<EnvLighting>();
	auto& gbufferData = blackboard->Get<GBufferData>();

	{
		lightView.mLightViewDepth = frameGraph->CreateTransient(
			GI::MemoryResourceDesc::RenderTarget2D(
				mRenderSize, 
				GI::Format::FORMAT_R24G8_TYPELESS, 
				GI::ResourceFlag::ALLOW_DEPTH_STENCIL, 
				"LightViewDepth")
			.SetInitState(GI::ResourceState::STATE_DEPTH_WRITE));

		cameraView.mMainViewDepth = frameGraph->CreateTransient(
			GI::MemoryResourceDesc::RenderTarget2D(
				mRenderSize,
				GI::Format::FORMAT_R32G8X24_TYPELESS,
				GI::ResourceFlag::ALLOW_DEPTH_STENCIL,
				"SceneDepthStencil")
			.SetInitState(GI::ResourceState::STATE_DEPTH_WRITE));

		cameraView.mShadowMask = frameGraph->CreateTransient(GI::MemoryResourceDesc::RenderTarget2D(
			mRenderSize, GI::Format::FORMAT_R16_FLOAT, GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS, "ShadowMask"));

		for (i32 i = 0; i < gbufferData.mGBuffers.size(); ++i)
		{
			gbufferData.mGBuffers[i] = frameGraph->CreateTransient(
				GI::MemoryResourceDesc::RenderTarget2D(
					mRenderSize,
					GI::Format::FORMAT_R16G16B16A16_UNORM,
					GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS,
					Utils::FormatString("GBuffer%d", i).c_str()));
		}
	}

	auto target = frameGraph->CreateTransient(GI::MemoryResourceDesc::RenderTarget2D(
		mRenderSize, 
		GI::Format::FORMAT_R11G11B10_FLOAT,
		GI::ResourceFlag::ALLOW_RENDER_TARGET, 
		"HdrRt"));

	if (!envLighting.mPanoramicSky.IsValid())
	{
		envLighting.mBRDFIntegrationMap = EnvironmentMap::GenerateIntegratedBRDF(frameGraph, 1024);

		const auto& srcSize = frameGraph->GetResourceDesc(mSkyTexture->GetResource()).GetSize();
		const Vec2u skyRtSize = { 1024, 1024 * srcSize.y() / srcSize.x() };
		
		envLighting.mPanoramicSky = frameGraph->CreatePermanent(
			GI::MemoryResourceDesc::RenderTarget2D(
				skyRtSize, 
				GI::Format::FORMAT_R32G32B32A32_FLOAT,
				GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS, 
				"PanoramicSkyRt"));

		RenderUtils::CopyTexture(frameGraph,
			envLighting.mPanoramicSky,
			Vec2f::Zero(), Vec2f{ skyRtSize.x(), skyRtSize.y() },
			mSkyTexture->GetResource(), 
			mNoMipMapLinearSampler, 
			Utils::FormatString("float4(color.xyz * %.2f, 1)", mSkyLightIntensity).c_str());

		envLighting.mIrradianceMap = EnvironmentMap::GenerateIrradianceMap(frameGraph, envLighting.mPanoramicSky, 8, 10);
		envLighting.mFilteredEnvMap = EnvironmentMap::GeneratePrefilteredEnvironmentMap(frameGraph, envLighting.mPanoramicSky, 1024);

		RenderUtils::GaussianBlur(frameGraph, envLighting.mPanoramicSky, envLighting.mPanoramicSky, 2);
	}

	//////////////////////////////////////////////////////////////////////////
	
	{
		//RENDER_EVENT(infra, LightViewDepth);

		frameGraph->AddClearPass("InitialLightViewDepth", {}, {}, 
			lightView.mLightViewDepth, true, sunLight.mLightViewProj.GetFarPlaneDeviceDepth(), true, 0);

		mTestModel->ForEach([&](const auto& node)
			{
				Geometry* geo = node.mContent.first.get();
				RenderMaterial* mat = node.mContent.second.get();

				if (geo && mat)
				{
					RenderGeometryDepthWithMaterial(frameGraph, geo, mat, node.mAbsTransform, lightView.mLightViewDepth);
				}
			});
	}

	//////////////////////////////////////////////////////////////////////////

	{
		//RENDER_EVENT(infra, GBuffer);
	
		frameGraph->AddClearPass("InitialGBufferAndDepth", 
			{ gbufferData.mGBuffers.begin(), gbufferData.mGBuffers.end() }, { 0.f, 0.f, 0.f, 1.f },
			cameraView.mMainViewDepth, true, cameraView.mCameraProj.GetFarPlaneDeviceDepth(), true, 0);

		mTestModel->ForEach([&](const auto& node)
			{
				Geometry* geo = node.mContent.first.get();
				RenderMaterial* mat = node.mContent.second.get();

				if (geo && mat)
				{
					RenderGeometryWithMaterial(frameGraph, geo, mat, node.mAbsTransform, gbufferData.mGBuffers, cameraView.mMainViewDepth);
				}
			});
	}

	//////////////////////////////////////////////////////////////////////////

	RenderShadowMask(frameGraph, cameraView.mShadowMask, lightView.mLightViewDepth, mNoMipMapLinearDepthCmpSampler, cameraView.mMainViewDepth, mNoMipMapLinearSampler);
	DeferredLighting(frameGraph, target);
	RenderSky(frameGraph, target, cameraView.mMainViewDepth);

	return target;
}

void WorldRenderer::RenderGBufferChannels(FrameGraphMutableResource& target)
{
	auto frameGraph = mRenderModule->GetFrameGraph();
	auto& gbufferData = frameGraph->GetBlackboard()->Get<GBufferData>();

	const std::pair<i32, const char*> gbufferSemantics[] =
	{
		{ 1, "float4(LinearToSrgb(color.xyz), 1)" },		// BaseColor,
		{ 0, "float4(LinearToSrgb(color.xyz), 1)" },		// Normal,
		{ 2, "float4(LinearToSrgb(color.yyy), 1)" },		// MetalMash,
		{ 0, "float4(LinearToSrgb(1.0 - color.www), 1)" },	// Roughness,
		{ 2, "float4(LinearToSrgb(color.zzz), 1)" },		// Reflection,
	};
	
	const auto& targetSize = frameGraph->GetResourceDesc(target).GetSize();
	const f32 width = f32(targetSize.x()) / Utils::GetArrayLength(gbufferSemantics);
	const f32 height = width / targetSize.x() * targetSize.y();

	for (u32 i = 0; i < Utils::GetArrayLength(gbufferSemantics); ++i)
	{
		const auto& [idx, unary] = gbufferSemantics[i];

		RenderUtils::CopyTexture(mRenderModule->GetFrameGraph(), 
			target, { i * width, 0.f }, { width, height }, 
			gbufferData.mGBuffers[idx], mNoMipMapLinearSampler, unary);
	}
}

void WorldRenderer::RenderShadowMaskChannel(FrameGraphMutableResource& target)
{
	auto frameGraph = mRenderModule->GetFrameGraph();
	const auto& targetSize = frameGraph->GetResourceDesc(target).GetSize();
	const f32 width = f32(targetSize.x()) * 0.25f;
	const f32 height = f32(targetSize.y()) * 0.25f;
	auto& cameraView = frameGraph->GetBlackboard()->Get<MainCameraState>();

	RenderUtils::CopyTexture(mRenderModule->GetFrameGraph(), 
		target, 
		{ 0.f, targetSize.y() - height }, { width, height },
		cameraView.mShadowMask,
		mNoMipMapLinearSampler, "float4(LinearToSrgb(color.xxx), 1)");
}

void WorldRenderer::RenderLightViewDepthChannel(FrameGraphMutableResource& target)
{
	auto frameGraph = mRenderModule->GetFrameGraph();
	const auto& targetSize = frameGraph->GetResourceDesc(target).GetSize();
	const f32 size = f32(targetSize.y()) * 0.25f;
	auto& lightView = frameGraph->GetBlackboard()->Get<LightViewData>();

	RenderUtils::CopyTexture(mRenderModule->GetFrameGraph(), 
		target, 
		{ 0.f, size }, { size, size },
		lightView.mLightViewDepth,
		mNoMipMapLinearSampler, "float4(LinearToSrgb(pow(color.xxx, 5)), 1)");
}

void WorldRenderer::DeferredLighting(FrameGraph* frameGraph, FrameGraphMutableResource& target)
{
	auto& camState = frameGraph->GetBlackboard()->Get<MainCameraState>();
	const auto& cameraProj = camState.mCameraProj;
	const auto& cameraTrans = camState.mCameraTrans;

	auto& sunLight = frameGraph->GetBlackboard()->Get<DirectionalLight>();
	auto& envLighting = frameGraph->GetBlackboard()->Get<EnvLighting>();
	auto& gbufferData = frameGraph->GetBlackboard()->Get<GBufferData>();

	//RENDER_EVENT(infra, DeferredLighting);

	const auto& mainDepthDesc = frameGraph->GetResourceDesc(camState.mMainViewDepth);
	const auto& dsSize = mainDepthDesc.GetSize();

	auto tempDepth = frameGraph->CreateTransient(
		GI::MemoryResourceDesc::RenderTarget2D(
			{ dsSize.x(), dsSize.y() }, 
			mainDepthDesc.GetFormat(),
			GI::ResourceFlag::ALLOW_DEPTH_STENCIL, 
			"TempMainDepth")
		.SetInitState(GI::ResourceState::STATE_DEPTH_WRITE));

	struct CopyData
	{
		FrameGraphMutableResource copyDest;
		FrameGraphResource copySrc;
	};

	frameGraph->AddPass<CopyData>("CopyMainDepth",
		[&](RenderPassBuilder& builder, CopyData& data)
		{
			data.copyDest = builder.WriteTex2DDsv(tempDepth).resource;
			data.copySrc = builder.ReadTex2DSrv(camState.mMainViewDepth).resource;
		},
		[](const CopyData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			infra->GetRecorder()->AddCopyOperation(resources.Get(data.copyDest.mId), resources.Get(data.copySrc.mId));
		});

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		std::array<SrvUsageFuture, 3> gBufferSrvs;
		SrvUsageFuture mainDepth;
		SrvUsageFuture shadowMask;
		GI::SamplerDesc lightingSceneSampler;
		SrvUsageFuture filteredEnvMapSrv;
		GI::SamplerDesc filteredEnvMapSampler;
		u16 filteredEnvMapMipCount;
		SrvUsageFuture irradianceMapSrv;
		GI::SamplerDesc panoramicSkySampler;
		SrvUsageFuture brdfIntegrationMapSrv;
		GI::SamplerDesc brdfIntegrationMapSampler;
		RtvUsageFuture target;
		DsvUsageFuture dsv;
		Vec3u targetSize;
		Vec3u dsSize;
	};

	frameGraph->AddPass<PassData>("DeferredLighting",
		[&](RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.ReadVbv(mQuad->GetVb(), mQuad->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(mQuad->GetIb(), mQuad->GetIbvDesc());
			data.lightingSceneSampler = builder.Read(mLightingSceneSampler);
			data.gBufferSrvs[0] = builder.ReadTex2DSrv(gbufferData.mGBuffers[0]);
			data.gBufferSrvs[1] = builder.ReadTex2DSrv(gbufferData.mGBuffers[1]);
			data.gBufferSrvs[2] = builder.ReadTex2DSrv(gbufferData.mGBuffers[2]);
			data.mainDepth = builder.ReadTex2DSrv(camState.mMainViewDepth);
			data.shadowMask = builder.ReadTex2DSrv(camState.mShadowMask);
			data.filteredEnvMapSrv = builder.ReadTex2DSrv(envLighting.mFilteredEnvMap);
			data.filteredEnvMapSampler = builder.Read(mFilteredEnvMapSampler);
			data.filteredEnvMapMipCount = frameGraph->GetResourceDesc(envLighting.mFilteredEnvMap).GetMipLevels();
			data.irradianceMapSrv = builder.ReadTex2DSrv(envLighting.mIrradianceMap);
			data.panoramicSkySampler = builder.Read(mPanoramicSkySampler);
			data.brdfIntegrationMapSrv = builder.ReadTex2DSrv(envLighting.mBRDFIntegrationMap);
			data.brdfIntegrationMapSampler = builder.Read(mBRDFIntegrationMapSampler);
			data.target = builder.WriteTex2DRtv(target);
			data.dsv = builder.ReadWriteTex2DDsv(tempDepth);
			data.targetSize = frameGraph->GetResourceDesc(target).GetSize();
		},
		[
			inputLayout = mQuad->mVertexElementDescs,
			indexCount = mQuad->mIndices.size(),
				cameraProj, cameraTrans, sunLight
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.mVsFile = "res/Shader/Lighting.hlsl";
			pass.mPsFile = "res/Shader/Lighting.hlsl";

			pass.mDepthStencilDesc
				.SetDepthEnable(false)
				.SetStencilEnable(true)
				.SetStencilReadMask(RenderUtils::WorldStencilMask_OpaqueObject)
				.SetStencilWriteMask(0);
			pass.mDepthStencilDesc.FrontFace
				.SetStencilFunc(GI::ComparisonFunction::EQUAL)
				.SetStencilPassOp(GI::StencilOp::KEEP)
				.SetStencilFailOp(GI::StencilOp::KEEP);
			pass.mDepthStencilDesc.BackFace
				.SetStencilFunc(GI::ComparisonFunction::EQUAL)
				.SetStencilPassOp(GI::StencilOp::KEEP)
				.SetStencilFailOp(GI::StencilOp::KEEP);

			const auto& targetSize = data.targetSize;
			pass.SetRtv(0, resources.Get(data.target));
			pass.SetDsv(resources.Get(data.dsv));
			pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
			pass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };
			pass.mStencilRef = RenderUtils::WorldStencilMask_OpaqueObject;

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
			pass.AddCbVar("FrustumInfo", Vec4f{ cameraProj.GetHalfFovHorizontal(), cameraProj.GetHalfFovVertical(), cameraProj.mNear, cameraProj.mFar });
			pass.AddSrv("GBuffer0", resources.Get(data.gBufferSrvs[0]));
			pass.AddSrv("GBuffer1", resources.Get(data.gBufferSrvs[1]));
			pass.AddSrv("GBuffer2", resources.Get(data.gBufferSrvs[2]));
			pass.AddSrv("ShadowMask", resources.Get(data.shadowMask));
			pass.AddSampler("ShadowMaskSampler", data.lightingSceneSampler);
			pass.AddSrv("SceneDepth", resources.Get(data.mainDepth));
			pass.AddSampler("GBufferSampler", data.lightingSceneSampler);

			pass.AddCbVar("InvViewMat", cameraTrans.ComputeInvViewMatrix());
			pass.AddCbVar("InvProjMat", cameraProj.ComputeInvProjectionMatrix());

			pass.AddSrv("PrefilteredEnvMap", resources.Get(data.filteredEnvMapSrv));
			pass.AddSampler("PrefilteredEnvMapSampler", data.filteredEnvMapSampler);
			pass.AddCbVar("PrefilteredInfo", Vec4f{ f32(data.filteredEnvMapMipCount), 0.f, 0.f, 0.f });

			pass.AddSrv("IrradianceMap", resources.Get(data.irradianceMapSrv));
			pass.AddSampler("IrradianceMapSampler", data.panoramicSkySampler);

			pass.AddSrv("BRDFIntegrationMap", resources.Get(data.brdfIntegrationMapSrv));
			pass.AddSampler("BRDFIntegrationMapSampler", data.brdfIntegrationMapSampler);

			pass.AddCbVar("CameraDir", cameraTrans.CamDirInWorldSpace());
			pass.AddCbVar("CameraPos", cameraTrans.CamPosInWorldSpace());
			pass.AddCbVar("LightDir", sunLight.mWorldTransform.CamDirInWorldSpace());
			pass.AddCbVar("LightColor", (sunLight.mLightColor * sunLight.mLightIntensity).eval());

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void WorldRenderer::RenderSky(FrameGraph* frameGraph, FrameGraphMutableResource& target, FrameGraphMutableResource& depth) const
{
	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		SrvUsageFuture panoramicSky;
		GI::SamplerDesc panoramicSampler;
		RtvUsageFuture target;
		DsvUsageFuture depth;
	};

	auto& camState = frameGraph->GetBlackboard()->Get<MainCameraState>();
	const auto& cameraProj = camState.mCameraProj;
	const auto& cameraTrans = camState.mCameraTrans;
	auto& envLighting = frameGraph->GetBlackboard()->Get<EnvLighting>();

	frameGraph->AddPass<PassData>("RenderSky",
		[&](RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.ReadVbv(mQuad->GetVb(), mQuad->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(mQuad->GetIb(), mQuad->GetIbvDesc());
			data.panoramicSky = builder.ReadTex2DSrv(envLighting.mPanoramicSky);
			data.panoramicSampler = builder.Read(mPanoramicSkySampler);
			data.target = builder.WriteTex2DRtv(target);
			data.depth = builder.ReadWriteTex2DDsv(depth);
		},
		[
			inputLayout = mQuad->mVertexElementDescs,
			indexCount = mQuad->mIndices.size(),
			targetSize = frameGraph->GetResourceDesc(target).GetSize(),
			camProj = camState.mCameraProj,
			camTrans = camState.mCameraTrans
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, Sky);

			GI::GraphicsPass pass;

			const Transformf& transform = Transformf(UniScalingf(1000.f));

			pass.mVsFile = "res/Shader/PanoramicSky.hlsl";
			pass.mPsFile = "res/Shader/PanoramicSky.hlsl";

			pass.mDepthStencilDesc
				.SetDepthEnable(false)
				.SetStencilEnable(true)
				.SetStencilReadMask(RenderUtils::WorldStencilMask_Scene & (~RenderUtils::WorldStencilMask_Sky))
				.SetStencilWriteMask(RenderUtils::WorldStencilMask_Sky);
			pass.mDepthStencilDesc.FrontFace
				.SetStencilFunc(GI::ComparisonFunction::EQUAL)
				.SetStencilDepthFailOp(GI::StencilOp::KEEP)
				.SetStencilPassOp(GI::StencilOp::REPLACE)
				.SetStencilFailOp(GI::StencilOp::KEEP);
			pass.mDepthStencilDesc.BackFace
				.SetStencilFunc(GI::ComparisonFunction::EQUAL)
				.SetStencilDepthFailOp(GI::StencilOp::KEEP)
				.SetStencilPassOp(GI::StencilOp::REPLACE)
				.SetStencilFailOp(GI::StencilOp::KEEP);

			pass.SetRtv(0, resources.Get(data.target));
			pass.SetDsv(resources.Get(data.depth));
			pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
			pass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };
			pass.mStencilRef = 0;

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
			pass.AddCbVar("FrustumInfo", Vec4f{ camProj.GetHalfFovHorizontal(), camProj.GetHalfFovVertical(), camProj.mNear, camProj.mFar });
			pass.AddCbVar("CameraDir", camTrans.CamDirInWorldSpace());
			pass.AddCbVar("InvViewMat", camTrans.ComputeInvViewMatrix());

			pass.AddSrv("PanoramicSky", resources.Get(data.panoramicSky));
			pass.AddSampler("PanoramicSkySampler", data.panoramicSampler);

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void WorldRenderer::RenderGeometryWithMaterial(FrameGraph* frameGraph, Geometry* geometry, RenderMaterial* material,
	const Transformf& transform,
	std::array<FrameGraphMutableResource, 3>& gbufferRtvs, FrameGraphMutableResource& depthView)
{
	//RENDER_EVENT(infra, WorldRenderer::RenderGeometryWithMaterial);
	
	auto& camState = frameGraph->GetBlackboard()->Get<MainCameraState>();
	const auto& cameraProj = camState.mCameraProj;
	const auto& cameraTrans = camState.mCameraTrans;

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		std::vector<GI::ShaderMacro> shaderMacros;
		std::vector<std::pair<std::string, SrvUsageFuture>> srvs;
		std::vector<std::pair<std::string, GI::SamplerDesc>> samplers;
		std::vector<std::pair<std::string, Vec4f>> cbvs;
		std::array<RtvUsageFuture, 3> gbufferRtvs;
		DsvUsageFuture depthView;
		Vec3u targetSize;
	};

	frameGraph->AddPass<PassData>("RenderGeometryWithMaterial",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
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
				if (attr.mTexture)
				{
					data.shaderMacros.push_back(GI::ShaderMacro{ paramName + "_USE_MAP", "" });

					auto res = attr.mTexture->GetResource();
					const auto& resDesc = frameGraph->GetResourceDesc(res);

					const auto& srvName = paramName + "Tex";
					const auto& srv = builder.Read(res,
							GI::SrvDesc{}
							.SetFormat(resDesc.GetFormat())
							.SetViewDimension(GI::GetSrvDimension(resDesc.GetDimension()))
							.SetTexture2D_MipLevels(resDesc.GetMipLevels()));
					data.srvs.emplace_back(srvName, srv);

					const auto& samplerName = paramName + "Sampler";
					const auto& sampler = attr.mSampler;
					data.samplers.emplace_back(samplerName, sampler);
				}
				else
				{
					data.cbvs.emplace_back(paramName + "ConstantValue", attr.mConstantValue);
				}
			}

			data.geoVertices = builder.ReadVbv(geometry->GetVb(), geometry->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(geometry->GetIb(), geometry->GetIbvDesc());
			for (i32 i = 0; i < gbufferRtvs.size(); ++i)
			{
				data.gbufferRtvs[i] = builder.WriteTex2DRtv(gbufferRtvs[i]);
			}
			data.depthView = builder.ReadWriteTex2DDsv(depthView);
			data.targetSize = frameGraph->GetResourceDesc(gbufferRtvs[0]).GetSize();
		},
		[
			inputLayout = geometry->mVertexElementDescs,
				indexCount = geometry->mIndices.size(),
				cameraProj, cameraTrans, transform
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.mVsFile = "res/Shader/GBufferPBRMat01.hlsl";
			pass.mPsFile = "res/Shader/GBufferPBRMat01.hlsl";

			pass.mRasterizerDesc
				.SetCullMode(GI::CullMode::NONE);

			pass.mDepthStencilDesc
				.SetDepthEnable(true)
				.SetDepthFunc(GI::ToDepthCompareFunc(cameraProj.GetNearerDepthCompare()))
				.SetStencilEnable(true)
				.SetStencilReadMask(RenderUtils::WorldStencilMask_Scene)
				.SetStencilWriteMask(RenderUtils::WorldStencilMask_OpaqueObject);
			pass.mDepthStencilDesc.FrontFace
				.SetStencilDepthFailOp(GI::StencilOp::KEEP)
				.SetStencilFailOp(GI::StencilOp::KEEP)
				.SetStencilPassOp(GI::StencilOp::REPLACE)
				.SetStencilFunc(GI::ComparisonFunction::ALWAYS);
			pass.mDepthStencilDesc.BackFace
				.SetStencilDepthFailOp(GI::StencilOp::KEEP)
				.SetStencilFailOp(GI::StencilOp::KEEP)
				.SetStencilPassOp(GI::StencilOp::REPLACE)
				.SetStencilFunc(GI::ComparisonFunction::ALWAYS);

			for (i32 i = 0; i < data.gbufferRtvs.size(); ++i)
			{
				pass.SetRtv(i, resources.Get(data.gbufferRtvs[i]));
			}
			pass.SetDsv(resources.Get(data.depthView));

			const auto& targetSize = data.targetSize;
			pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
			pass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };
			pass.mStencilRef = RenderUtils::WorldStencilMask_OpaqueObject;

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

			pass.AddCbVar("worldMat", transform.matrix());
			pass.AddCbVar("viewMat", cameraTrans.ComputeViewMatrix());
			pass.AddCbVar("projMat", cameraProj.ComputeProjectionMatrix());

			pass.mShaderMacros = data.shaderMacros;
			for (const auto& [n, srv] : data.srvs) { pass.AddSrv(n, resources.Get(srv)); }
			for (const auto& [n, sampler] : data.samplers) { pass.AddSampler(n, sampler); }
			for (const auto& [n, v] : data.cbvs) { pass.AddCbVar(n, v); }
			
			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void WorldRenderer::RenderGeometryDepthWithMaterial(
	FrameGraph* frameGraph, 
	Geometry* geometry, RenderMaterial* material,
	const Transformf& transform,
	FrameGraphMutableResource& depth)
{
	//RENDER_EVENT(infra, WorldRenderer::RenderGeometryDepthWithMaterial);

	auto& camState = frameGraph->GetBlackboard()->Get<DirectionalLight>();
	const auto& cameraProj = camState.mLightViewProj;
	const auto& cameraTrans = camState.mWorldTransform;

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		std::vector<std::pair<std::string, SrvUsageFuture>> srvs;
		std::vector<std::pair<std::string, GI::SamplerDesc>> samplers;
		DsvUsageFuture depthView;
	};

	frameGraph->AddPass<PassData>("RenderGeometryDepthWithMaterial",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			const char* paramName = "BaseColorTex";
			const auto& attr = material->mMatAttriSlots[TextureUsage_BaseColor];
			if (attr.mTexture)
			{
				data.srvs.emplace_back(paramName, builder.ReadTex2DSrv(attr.mTexture->GetResource()));
				data.samplers.emplace_back(std::string(paramName) + "Sampler", builder.Read(attr.mSampler));
			}

			data.geoVertices = builder.ReadVbv(geometry->GetVb(), geometry->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(geometry->GetIb(), geometry->GetIbvDesc());
			data.depthView = builder.ReadWriteTex2DDsv(depth);
		},
		[
			inputLayout = geometry->mVertexElementDescs,
			indexCount = geometry->mIndices.size(),
			cameraProj, cameraTrans, transform
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.mVsFile = "res/Shader/GeometryDepth.hlsl";
			pass.mPsFile = "res/Shader/GeometryDepth.hlsl";

			pass.mRasterizerDesc
				.SetCullMode(GI::CullMode::NONE)
				.SetDepthBias(10000)
				.SetSlopeScaledDepthBias(10);

			pass.mDepthStencilDesc
				.SetDepthEnable(true)
				.SetDepthFunc(GI::ToDepthCompareFunc(cameraProj.GetNearerDepthCompare()))
				.SetStencilEnable(false);

			const auto& dsv = resources.Get(data.depthView);
			pass.SetDsv(dsv);

			const auto& targetSize = dsv.GetResource()->GetSize();
			pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
			pass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };
			pass.mStencilRef = RenderUtils::WorldStencilMask_OpaqueObject;

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

			pass.AddCbVar("worldMat", transform.matrix());
			pass.AddCbVar("viewMat", cameraTrans.ComputeViewMatrix());
			pass.AddCbVar("projMat", cameraProj.ComputeProjectionMatrix());

			for (const auto& [n, srv] : data.srvs) { pass.AddSrv(n, resources.Get(srv)); }
			for (const auto& [n, sampler] : data.samplers) { pass.AddSampler(n, sampler); }

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void WorldRenderer::RenderShadowMask(FrameGraph* frameGraph, 
	FrameGraphMutableResource& shadowMask,
	FrameGraphResource lightViewDepth, const GI::SamplerDesc& lightViewDepthSampler,
	FrameGraphResource cameraViewDepth, const GI::SamplerDesc& cameraViewDepthSampler)
{
	auto* blackboard = frameGraph->GetBlackboard();

	auto& camState = blackboard->Get<MainCameraState>();
	const auto& cameraProj = camState.mCameraProj;
	const auto& cameraTrans = camState.mCameraTrans;

	auto& lightView = blackboard->Get<DirectionalLight>();
	const auto& lightViewTrans = lightView.mWorldTransform;
	const auto& lightViewProj = lightView.mLightViewProj;

	//RENDER_EVENT(infra, ShadowMask);

	static Geometry* geometry = Geometry::GenerateQuad()->CreateAndInitialResource(frameGraph);

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		SrvUsageFuture lightViewDepth;
		GI::SamplerDesc lightViewDepthSampler;
		SrvUsageFuture cameraViewDepth;
		GI::SamplerDesc cameraViewDepthSampler;
		RtvUsageFuture shadowMask;
		Vec3u targetSize;
	};

	frameGraph->AddPass<PassData>("RenderShadowMask",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.ReadVbv(geometry->GetVb(), geometry->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(geometry->GetIb(), geometry->GetIbvDesc());
			data.lightViewDepth = builder.ReadTex2DSrv(lightViewDepth);
			data.lightViewDepthSampler = builder.Read(lightViewDepthSampler);
			data.cameraViewDepth = builder.ReadTex2DSrv(cameraViewDepth);
			data.cameraViewDepthSampler = builder.Read(cameraViewDepthSampler);
			data.targetSize = frameGraph->GetResourceDesc(shadowMask).GetSize();
			data.shadowMask = builder.WriteTex2DRtv(shadowMask);
		},
		[
			inputLayout = geometry->mVertexElementDescs,
			indexCount = geometry->mIndices.size(),
			cameraTrans, cameraProj,
			lightViewTrans, lightViewProj
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.mVsFile = "res/Shader/ConstructShadowMask.hlsl";
			pass.mPsFile = "res/Shader/ConstructShadowMask.hlsl";

			pass.mDepthStencilDesc
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.SetRtv(0, resources.Get(data.shadowMask));

			const auto& targetSize = data.targetSize;
			pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
			pass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddSrv("LightViewDepth", resources.Get(data.lightViewDepth));
			pass.AddSampler("LightViewDepthSampler", data.lightViewDepthSampler);
			pass.AddSrv("CameraViewDepth", resources.Get(data.cameraViewDepth));
			pass.AddSampler("CameraViewDepthSampler", data.cameraViewDepthSampler);

			pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
			pass.AddCbVar("FrustumInfo", Vec4f{ cameraProj.GetHalfFovHorizontal(), cameraProj.GetHalfFovVertical(), cameraProj.mNear, cameraProj.mFar });

			pass.AddCbVar("CameraViewMat", cameraTrans.ComputeViewMatrix());
			pass.AddCbVar("CameraInvViewMat", cameraTrans.ComputeInvViewMatrix());
			pass.AddCbVar("CameraProjMat", cameraProj.ComputeProjectionMatrix());
			pass.AddCbVar("CameraInvProjMat", cameraProj.ComputeInvProjectionMatrix());

			pass.AddCbVar("LightViewMat", lightViewTrans.ComputeViewMatrix());
			pass.AddCbVar("LightProjMat", lightViewProj.ComputeProjectionMatrix());

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

