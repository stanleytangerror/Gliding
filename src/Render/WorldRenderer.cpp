#include "Render/RenderPch.h"
#include "WorldRenderer.h"
#include "RenderModule.h"
#include "Geometry.h"
#include "Texture.h"
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
	const std::string assetDir = R"(D:\Assets\)";

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

	auto skyTexPath = assetDir + R"(Panorama_of_Marienplatz.dds)";
	auto skyTexContent = Utils::LoadFileContent(skyTexPath.c_str());
	mSkyTexture = std::make_unique<FileTexture>(frameGraph, skyTexPath.c_str(), std::span(skyTexContent));

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

	mGeometryCollection->CreateAndInitialResource(frameGraph);

	//auto model = DeserializeFromBytes<ModelProcess::Model>(Utils::LoadFileContent((assetDir + R"(monobike_derivative\build.bin)").c_str()));
	//auto transform = Transformf::Identity();

	//auto model = DeserializeFromBytes<ModelProcess::Model>(Utils::LoadFileContent((assetDir + R"(seamless_pbr_texture_metal_01\build.bin)").c_str()));
	//auto transform = Transformf(UniScalingf(25.f)) * Translationf(0.f, 0.f, -1.f);

	auto model = DeserializeFromBytes<ModelProcess::Model>(Utils::LoadFileContent((assetDir + R"(free_1975_porsche_911_930_turbo\build.bin)").c_str()));
	auto transform = Transformf(UniScalingf(25.f)) * Translationf(0.f, 0.f, -1.f);

	//auto model = DeserializeFromBytes<ModelProcess::Model>(Utils::LoadFileContent((assetDir + R"(hintze-hall_-_vr_tour\build.bin)").c_str()));
	//auto transform = Transformf(UniScalingf(5.f));

	//auto model = DeserializeFromBytes<ModelProcess::Model>(Utils::LoadFileContent((assetDir + R"(2019_porsche_935_martini_racing-gltf\build.bin)").c_str()));
	//auto transform = Transformf(UniScalingf(30.f));

	//auto model = DeserializeFromBytes<ModelProcess::Model>(Utils::LoadFileContent((assetDir + R"(Bistro_v5_2\build.bin)").c_str()));
	//auto transform = Transformf(UniScalingf(1.f));
	 
	//auto model = DeserializeFromBytes<ModelProcess::Model>(Utils::LoadFileContent((assetDir + R"(slum_house\build.bin)"));
	//auto transform = Transformf(UniScalingf(10.f));

	//mTestModel.reset(RenderUtils::GenerateMaterialProbes(frameGraph));
	//auto transform = Transformf(UniScalingf(10.f));

	mScene->AddModelData(frameGraph, model, transform);
}

WorldRenderer::~WorldRenderer()
{

}

void WorldRenderer::TickFrame(Timer* timer)
{
	mScene->UpdateAbsoluteTransforms();
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

	auto quad = mGeometryCollection->GetQuad();

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
		envLighting.mBRDFIntegrationMap = EnvironmentMap::GenerateIntegratedBRDF(quad, frameGraph, 1024);

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
			quad,
			mSkyTexture->GetResource(), 
			mNoMipMapLinearSampler, 
			Utils::FormatString("float4(color.xyz * %.2f, 1)", mSkyLightIntensity).c_str());

		envLighting.mIrradianceMap = EnvironmentMap::GenerateIrradianceMap(quad, frameGraph, envLighting.mPanoramicSky, 8, 10);
		envLighting.mFilteredEnvMap = EnvironmentMap::GeneratePrefilteredEnvironmentMap(quad, frameGraph, envLighting.mPanoramicSky, 1024);

		RenderUtils::GaussianBlur(quad, frameGraph, envLighting.mPanoramicSky, envLighting.mPanoramicSky, 2);
	}

	//////////////////////////////////////////////////////////////////////////
	
	{
		//RENDER_EVENT(infra, LightViewDepth);

		frameGraph->AddClearPass("InitialLightViewDepth", {}, {}, 
			lightView.mLightViewDepth, true, sunLight.mLightViewProj.GetFarPlaneDeviceDepth(), true, 0);

		mScene->ForEachMeshInstance([frameGraph, &lightView](const auto& transform, const auto& geo, const auto& mat)
			{
				RenderGeometryDepthWithMaterial(frameGraph, geo, mat, transform, lightView.mLightViewDepth);
			});
	}

	//////////////////////////////////////////////////////////////////////////

	{
		//RENDER_EVENT(infra, GBuffer);
	
		frameGraph->AddClearPass("InitialGBufferAndDepth", 
			{ gbufferData.mGBuffers.begin(), gbufferData.mGBuffers.end() }, { 0.f, 0.f, 0.f, 1.f },
			cameraView.mMainViewDepth, true, cameraView.mCameraProj.GetFarPlaneDeviceDepth(), true, 0);

		mScene->ForEachMeshInstance([frameGraph, &gbufferData, &cameraView](const auto& transform, const auto& geo, const auto& mat)
			{
				RenderGeometryWithMaterial(frameGraph, geo, mat, transform, gbufferData.mGBuffers, cameraView.mMainViewDepth);
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
	auto quad = mGeometryCollection->GetQuad();

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
			target, { i * width, 0.f }, { width, height }, quad,
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
	auto quad = mGeometryCollection->GetQuad();

	RenderUtils::CopyTexture(mRenderModule->GetFrameGraph(), 
		target, 
		{ 0.f, targetSize.y() - height }, { width, height },
		quad,
		cameraView.mShadowMask,
		mNoMipMapLinearSampler, "float4(LinearToSrgb(color.xxx), 1)");
}

void WorldRenderer::RenderLightViewDepthChannel(FrameGraphMutableResource& target)
{
	auto frameGraph = mRenderModule->GetFrameGraph();
	const auto& targetSize = frameGraph->GetResourceDesc(target).GetSize();
	const f32 size = f32(targetSize.y()) * 0.25f;
	auto& lightView = frameGraph->GetBlackboard()->Get<LightViewData>();
	auto quad = mGeometryCollection->GetQuad();

	RenderUtils::CopyTexture(mRenderModule->GetFrameGraph(), 
		target, 
		{ 0.f, size }, { size, size },
		quad,
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

	auto quad = mGeometryCollection->GetQuad();

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
			data.geoVertices = builder.ReadVbv(quad->GetVb(), quad->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(quad->GetIb(), quad->GetIbvDesc());
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
			inputLayout = quad->mVertexElementDescs,
			indexCount = quad->mIndices.size(),
				cameraProj, cameraTrans, sunLight
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.SetShader("Lighting");

			pass.SetupDepthStencil()
				.SetDepthEnable(false)
				.SetStencilEnable(true)
				.SetStencilReadMask(RenderUtils::WorldStencilMask_OpaqueObject)
				.SetStencilWriteMask(0);
			pass.SetupDepthStencil().FrontFace
				.SetStencilFunc(GI::ComparisonFunction::EQUAL)
				.SetStencilPassOp(GI::StencilOp::KEEP)
				.SetStencilFailOp(GI::StencilOp::KEEP);
			pass.SetupDepthStencil().BackFace
				.SetStencilFunc(GI::ComparisonFunction::EQUAL)
				.SetStencilPassOp(GI::StencilOp::KEEP)
				.SetStencilFailOp(GI::StencilOp::KEEP);

			const auto& targetSize = data.targetSize;
			pass.SetRtv(0, resources.Get(data.target));
			pass.SetDsv(resources.Get(data.dsv));
			pass.SetViewPortAndScissorRectToFullRt();

			pass.SetStencilRef(RenderUtils::WorldStencilMask_OpaqueObject);

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCb4f("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
			pass.AddCb4f("FrustumInfo", Vec4f{ cameraProj.GetHalfFovHorizontal(), cameraProj.GetHalfFovVertical(), cameraProj.mNear, cameraProj.mFar });
			pass.AddSrv("GBuffer0", resources.Get(data.gBufferSrvs[0]));
			pass.AddSrv("GBuffer1", resources.Get(data.gBufferSrvs[1]));
			pass.AddSrv("GBuffer2", resources.Get(data.gBufferSrvs[2]));
			pass.AddSrv("ShadowMask", resources.Get(data.shadowMask));
			pass.AddSampler("ShadowMaskSampler", data.lightingSceneSampler);
			pass.AddSrv("SceneDepth", resources.Get(data.mainDepth));
			pass.AddSampler("GBufferSampler", data.lightingSceneSampler);

			pass.AddCb44f("InvViewMat", cameraTrans.ComputeInvViewMatrix());
			pass.AddCb44f("InvProjMat", cameraProj.ComputeInvProjectionMatrix());

			pass.AddSrv("PrefilteredEnvMap", resources.Get(data.filteredEnvMapSrv));
			pass.AddSampler("PrefilteredEnvMapSampler", data.filteredEnvMapSampler);
			pass.AddCb4f("PrefilteredInfo", Vec4f{ f32(data.filteredEnvMapMipCount), 0.f, 0.f, 0.f });

			pass.AddSrv("IrradianceMap", resources.Get(data.irradianceMapSrv));
			pass.AddSampler("IrradianceMapSampler", data.panoramicSkySampler);

			pass.AddSrv("BRDFIntegrationMap", resources.Get(data.brdfIntegrationMapSrv));
			pass.AddSampler("BRDFIntegrationMapSampler", data.brdfIntegrationMapSampler);

			pass.AddCb3f("CameraDir", cameraTrans.CamDirInWorldSpace());
			pass.AddCb3f("CameraPos", cameraTrans.CamPosInWorldSpace());
			pass.AddCb3f("LightDir", sunLight.mWorldTransform.CamDirInWorldSpace());
			pass.AddCb3f("LightColor", (sunLight.mLightColor * sunLight.mLightIntensity).eval());

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

	auto quad = mGeometryCollection->GetQuad();

	frameGraph->AddPass<PassData>("RenderSky",
		[&](RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.ReadVbv(quad->GetVb(), quad->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(quad->GetIb(), quad->GetIbvDesc());
			data.panoramicSky = builder.ReadTex2DSrv(envLighting.mPanoramicSky);
			data.panoramicSampler = builder.Read(mPanoramicSkySampler);
			data.target = builder.WriteTex2DRtv(target);
			data.depth = builder.ReadWriteTex2DDsv(depth);
		},
		[
			inputLayout = quad->mVertexElementDescs,
			indexCount = quad->mIndices.size(),
			targetSize = frameGraph->GetResourceDesc(target).GetSize(),
			camProj = camState.mCameraProj,
			camTrans = camState.mCameraTrans
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, Sky);

			GI::GraphicsPass pass;

			const Transformf& transform = Transformf(UniScalingf(1000.f));

			pass.SetShader("PanoramicSky");

			pass.SetupDepthStencil()
				.SetDepthEnable(false)
				.SetStencilEnable(true)
				.SetStencilReadMask(RenderUtils::WorldStencilMask_Scene & (~RenderUtils::WorldStencilMask_Sky))
				.SetStencilWriteMask(RenderUtils::WorldStencilMask_Sky);
			pass.SetupDepthStencil().FrontFace
				.SetStencilFunc(GI::ComparisonFunction::EQUAL)
				.SetStencilDepthFailOp(GI::StencilOp::KEEP)
				.SetStencilPassOp(GI::StencilOp::REPLACE)
				.SetStencilFailOp(GI::StencilOp::KEEP);
			pass.SetupDepthStencil().BackFace
				.SetStencilFunc(GI::ComparisonFunction::EQUAL)
				.SetStencilDepthFailOp(GI::StencilOp::KEEP)
				.SetStencilPassOp(GI::StencilOp::REPLACE)
				.SetStencilFailOp(GI::StencilOp::KEEP);

			pass.SetRtv(0, resources.Get(data.target));
			pass.SetDsv(resources.Get(data.depth));
			pass.SetViewPortAndScissorRectToFullRt();

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCb4f("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
			pass.AddCb4f("FrustumInfo", Vec4f{ camProj.GetHalfFovHorizontal(), camProj.GetHalfFovVertical(), camProj.mNear, camProj.mFar });
			pass.AddCb3f("CameraDir", camTrans.CamDirInWorldSpace());
			pass.AddCb44f("InvViewMat", camTrans.ComputeInvViewMatrix());

			pass.AddSrv("PanoramicSky", resources.Get(data.panoramicSky));
			pass.AddSampler("PanoramicSkySampler", data.panoramicSampler);

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void WorldRenderer::RenderGeometryWithMaterial(FrameGraph* frameGraph, const Geometry* geometry, RenderMaterial* material,
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
			const auto& normalChannel = material->mNormalChannel;
			if (auto tex = normalChannel.mTexture)
			{
				data.shaderMacros.push_back(GI::ShaderMacro{ "Normal_USE_MAP", "" });
				data.srvs.emplace_back("NormalTex", builder.ReadTex2DSrv(tex->GetResource()));
				data.samplers.emplace_back("NormalSampler", normalChannel.mSampler);
			}
			else
			{
				data.cbvs.emplace_back("NormalConstantValue", Vec4f{ normalChannel.mNormalConstant.x(), normalChannel.mNormalConstant.y(), normalChannel.mNormalConstant.z(), 0.f });
			}

			const auto& diffuseChannel = material->mDiffuseChannel;
			if (auto tex = diffuseChannel.mTexture)
			{
				data.shaderMacros.push_back(GI::ShaderMacro{ "Diffuse_USE_MAP", "" });
				data.srvs.emplace_back("DiffuseTex", builder.ReadTex2DSrv(tex->GetResource()));
				data.samplers.emplace_back("DiffuseSampler", diffuseChannel.mSampler);
			}
			else
			{
				data.cbvs.emplace_back("DiffuseConstantValue", diffuseChannel.mDiffuseConstant);
			}
			
			const auto& baseColorChannel = material->mBaseColorChannel;
			if (auto tex = baseColorChannel.mTexture)
			{
				data.shaderMacros.push_back(GI::ShaderMacro{ "BaseColor_USE_MAP", "" });
				data.srvs.emplace_back("BaseColorTex", builder.ReadTex2DSrv(tex->GetResource()));
				data.samplers.emplace_back("BaseColorSampler", baseColorChannel.mSampler);
			}
			else
			{
				data.cbvs.emplace_back("BaseColorConstantValue", baseColorChannel.mColor);
			}

			const auto& metallicRoughnessChannel = material->mMetallicRoughnessChannel;
			if (auto tex = metallicRoughnessChannel.mTexture)
			{
				data.shaderMacros.push_back(GI::ShaderMacro{ "MetallicRoughness_USE_MAP", "" });
				data.srvs.emplace_back("MetallicRoughnessTex", builder.ReadTex2DSrv(tex->GetResource()));
				data.samplers.emplace_back("MetallicRoughnessSampler", metallicRoughnessChannel.mSampler);
			}
			else
			{
				data.cbvs.emplace_back("MetallicRoughnessConstantValue", Vec4f{
					0.f,
					metallicRoughnessChannel.mRoughnessFactor,
					metallicRoughnessChannel.mMetallicFactor,
					0.f });
			}

			const auto& specularGlossinessChannel = material->mSpecularGlossinessChannel;
			if (auto tex = specularGlossinessChannel.mTexture)
			{
				data.shaderMacros.push_back(GI::ShaderMacro{ "SpecularGlossiness_USE_MAP", "" });
				data.srvs.emplace_back("SpecularGlossinessTex", builder.ReadTex2DSrv(tex->GetResource()));
				data.samplers.emplace_back("SpecularGlossinessSampler", metallicRoughnessChannel.mSampler);
			}
			else
			{
				data.cbvs.emplace_back("SpecularGlossinessConstantValue", Vec4f{
					specularGlossinessChannel.mSpecularConstant.x(),
					specularGlossinessChannel.mSpecularConstant.y(),
					specularGlossinessChannel.mSpecularConstant.z(),
					specularGlossinessChannel.mGlossinessConstant,
					});
			}


			if (geometry->mHasTangent) { data.shaderMacros.push_back(GI::ShaderMacro{ "HAS_TANGENT", "" }); }
			if (geometry->mHasBiTangent) { data.shaderMacros.push_back(GI::ShaderMacro{ "HAS_BITANGENT", "" }); }
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

			pass.SetShader("GBufferPBRMat01", data.shaderMacros);

			pass.SetupRasterizer()
				.SetCullMode(GI::CullMode::NONE);

			pass.SetupDepthStencil()
				.SetDepthEnable(true)
				.SetDepthFunc(GI::ToDepthCompareFunc(cameraProj.GetNearerDepthCompare()))
				.SetStencilEnable(true)
				.SetStencilReadMask(RenderUtils::WorldStencilMask_Scene)
				.SetStencilWriteMask(RenderUtils::WorldStencilMask_OpaqueObject);
			pass.SetupDepthStencil().FrontFace
				.SetStencilDepthFailOp(GI::StencilOp::KEEP)
				.SetStencilFailOp(GI::StencilOp::KEEP)
				.SetStencilPassOp(GI::StencilOp::REPLACE)
				.SetStencilFunc(GI::ComparisonFunction::ALWAYS);
			pass.SetupDepthStencil().BackFace
				.SetStencilDepthFailOp(GI::StencilOp::KEEP)
				.SetStencilFailOp(GI::StencilOp::KEEP)
				.SetStencilPassOp(GI::StencilOp::REPLACE)
				.SetStencilFunc(GI::ComparisonFunction::ALWAYS);

			for (i32 i = 0; i < data.gbufferRtvs.size(); ++i)
			{
				pass.SetRtv(i, resources.Get(data.gbufferRtvs[i]));
			}
			pass.SetDsv(resources.Get(data.depthView));
			pass.SetViewPortAndScissorRectToFullRt();

			pass.SetStencilRef(RenderUtils::WorldStencilMask_OpaqueObject);

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			const auto& targetSize = data.targetSize;
			pass.AddCb4f("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

			pass.AddCb44f("worldMat", transform.matrix());
			pass.AddCb44f("viewMat", cameraTrans.ComputeViewMatrix());
			pass.AddCb44f("projMat", cameraProj.ComputeProjectionMatrix());

			for (const auto& [n, srv] : data.srvs) { pass.AddSrv(n, resources.Get(srv)); }
			for (const auto& [n, sampler] : data.samplers) { pass.AddSampler(n, sampler); }
			for (const auto& [n, v] : data.cbvs) { pass.AddCb4f(n, v); }
			
			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void WorldRenderer::RenderGeometryDepthWithMaterial(
	FrameGraph* frameGraph, 
	const Geometry* geometry, RenderMaterial* material,
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
		std::vector<GI::ShaderMacro> shaderMacros;
		std::vector<std::pair<std::string, SrvUsageFuture>> srvs;
		std::vector<std::pair<std::string, GI::SamplerDesc>> samplers;
		DsvUsageFuture depthView;
		Vec3u targetSize;
	};

	frameGraph->AddPass<PassData>("RenderGeometryDepthWithMaterial",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			const auto& baseColorChannel = material->mBaseColorChannel;
			if (auto tex = baseColorChannel.mTexture)
			{
				data.srvs.emplace_back("BaseColorTex", builder.ReadTex2DSrv(tex->GetResource()));
				data.samplers.emplace_back("BaseColorSampler", baseColorChannel.mSampler);
			}

			if (geometry->mHasTangent) { data.shaderMacros.push_back(GI::ShaderMacro{ "HAS_TANGENT", "" }); }
			if (geometry->mHasBiTangent) { data.shaderMacros.push_back(GI::ShaderMacro{ "HAS_BITANGENT", "" }); }
			data.geoVertices = builder.ReadVbv(geometry->GetVb(), geometry->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(geometry->GetIb(), geometry->GetIbvDesc());
			data.depthView = builder.ReadWriteTex2DDsv(depth);
			data.targetSize = frameGraph->GetResourceDesc(depth).GetSize();
		},
		[
			inputLayout = geometry->mVertexElementDescs,
			indexCount = geometry->mIndices.size(),
			cameraProj, cameraTrans, transform
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.SetShader("GeometryDepth", data.shaderMacros);

			pass.SetupRasterizer()
				.SetCullMode(GI::CullMode::NONE)
				.SetDepthBias(10000)
				.SetSlopeScaledDepthBias(10);

			pass.SetupDepthStencil()
				.SetDepthEnable(true)
				.SetDepthFunc(GI::ToDepthCompareFunc(cameraProj.GetNearerDepthCompare()))
				.SetStencilEnable(false);

			pass.SetDsv(resources.Get(data.depthView));
			pass.SetViewPortAndScissorRectToFullDepth();
			
			pass.SetStencilRef(RenderUtils::WorldStencilMask_OpaqueObject);

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCb4f("RtSize", Vec4f{ f32(data.targetSize.x()), f32(data.targetSize.y()), 1.f / data.targetSize.x(), 1.f / data.targetSize.y() });

			pass.AddCb44f("worldMat", transform.matrix());
			pass.AddCb44f("viewMat", cameraTrans.ComputeViewMatrix());
			pass.AddCb44f("projMat", cameraProj.ComputeProjectionMatrix());

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

	auto quad = mGeometryCollection->GetQuad();

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
		[
			frameGraph, quad,
			&lightViewDepth, &lightViewDepthSampler,
			&cameraViewDepth, &cameraViewDepthSampler,
			&shadowMask
		]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.ReadVbv(quad->GetVb(), quad->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(quad->GetIb(), quad->GetIbvDesc());
			data.lightViewDepth = builder.ReadTex2DSrv(lightViewDepth);
			data.lightViewDepthSampler = builder.Read(lightViewDepthSampler);
			data.cameraViewDepth = builder.ReadTex2DSrv(cameraViewDepth);
			data.cameraViewDepthSampler = builder.Read(cameraViewDepthSampler);
			data.targetSize = frameGraph->GetResourceDesc(shadowMask).GetSize();
			data.shadowMask = builder.WriteTex2DRtv(shadowMask);
		},
		[
			inputLayout = quad->mVertexElementDescs,
			indexCount = quad->mIndices.size(),
			cameraTrans, cameraProj,
			lightViewTrans, lightViewProj
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.SetShader("ConstructShadowMask");

			pass.SetupDepthStencil()
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.SetRtv(0, resources.Get(data.shadowMask));
			pass.SetViewPortAndScissorRectToFullRt();

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddSrv("LightViewDepth", resources.Get(data.lightViewDepth));
			pass.AddSampler("LightViewDepthSampler", data.lightViewDepthSampler);
			pass.AddSrv("CameraViewDepth", resources.Get(data.cameraViewDepth));
			pass.AddSampler("CameraViewDepthSampler", data.cameraViewDepthSampler);

			const auto& targetSize = data.targetSize;
			pass.AddCb4f("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
			pass.AddCb4f("FrustumInfo", Vec4f{ cameraProj.GetHalfFovHorizontal(), cameraProj.GetHalfFovVertical(), cameraProj.mNear, cameraProj.mFar });

			pass.AddCb44f("CameraViewMat", cameraTrans.ComputeViewMatrix());
			pass.AddCb44f("CameraInvViewMat", cameraTrans.ComputeInvViewMatrix());
			pass.AddCb44f("CameraProjMat", cameraProj.ComputeProjectionMatrix());
			pass.AddCb44f("CameraInvProjMat", cameraProj.ComputeInvProjectionMatrix());

			pass.AddCb44f("LightViewMat", lightViewTrans.ComputeViewMatrix());
			pass.AddCb44f("LightProjMat", lightViewProj.ComputeProjectionMatrix());

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

