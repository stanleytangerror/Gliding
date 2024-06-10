#include "RenderPch.h"
#include "WorldRenderer.h"
#include "RenderModule.h"
#include "RenderTarget.h"
#include "Geometry.h"
#include "Texture.h"
#include "World/Scene.h"
#include "RenderMaterial.h"
#include "RenderUtils.h"
#include "Light.h"
#include "EnvironmentMap.h"
#include "FrameGraph.h"

WorldRenderer::WorldRenderer(RenderModule* renderModule, const Vec2u& renderSize)
	: mRenderModule(renderModule)
	, mRenderSize(renderSize)
{
	auto infra = mRenderModule->GetGraphicsInfra();
	auto* blackboard = renderModule->GetFrameGraph()->GetBlackboard();

	auto& cam = blackboard->Add<MainCameraState>();
	{
		cam.mCameraTrans.MoveCamera(200.f * Math::Axis3DDir<f32>(Math::Axis3D_Yn));
		cam.mCameraProj.mFovHorizontal = Math::DegreeToRadian(90.f);
		cam.mCameraProj.mAspectRatio = f32(renderSize.x()) / renderSize.y();
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

	mSphere.reset(Geometry::GenerateSphere(40));
	mQuad.reset(Geometry::GenerateQuad());
	
	const char* skyTexPath = R"(D:\Assets\Panorama_of_Marienplatz.dds)";
	mSkyTexture = std::make_unique<FileTexture>(infra, skyTexPath, Utils::LoadFileContent(skyTexPath));

	mPanoramicSkySampler
		.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
		.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP });

	mLightingSceneSampler
		.SetFilter(GI::Filter::MIN_MAG_POINT_MIP_LINEAR)
		.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP });
	mNoMipMapLinearSampler
		.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
		.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP });
	mFilteredEnvMapSampler
		.SetFilter(GI::Filter::MIN_MAG_MIP_LINEAR)
		.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::CLAMP, GI::TextureAddressMode::CLAMP });

	mBRDFIntegrationMapSampler
		.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
		.SetAddress({ GI::TextureAddressMode::CLAMP, GI::TextureAddressMode::CLAMP, GI::TextureAddressMode::CLAMP });

	const f32 farPlaneDeviceDepth = sunLight.mLightViewProj.GetFarPlaneDeviceDepth();
		
	mNoMipMapLinearDepthCmpSampler
		.SetFilter(GI::Filter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT)
		.SetAddress({ GI::TextureAddressMode::BORDER, GI::TextureAddressMode::BORDER, GI::TextureAddressMode::BORDER })
		.SetBorderColor(Vec4f::Ones() * farPlaneDeviceDepth)
		.SetComparisonFunc(GI::ComparisonFunction::LESS_EQUAL);

	for (i32 i = 0; i < mGBuffers.size(); ++i)
	{
		mGBuffers[i] = infra->CreateMemoryResource(
			GI::MemoryResourceDesc()
			.SetDimension(GI::ResourceDimension::TEXTURE2D)
			.SetWidth(mRenderSize.x())
			.SetHeight(mRenderSize.y())
			.SetDepthOrArraySize(1)
			.SetMipLevels(1)
			.SetFormat(GI::Format::FORMAT_R16G16B16A16_UNORM)
			.SetLayout(GI::TextureLayout::LAYOUT_UNKNOWN)
			.SetFlags(GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS)
			.SetInitState(GI::ResourceState::STATE_RENDER_TARGET)
			.SetName(Utils::FormatString("GBuffer%d", i).c_str())
			.SetHeapType(GI::HeapType::DEFAULT)); 

		mGBufferSrvs[i] = GI::SrvUsage(mGBuffers[i]);
		mGBufferSrvs[i]
			.SetFormat(GI::Format::FORMAT_R16G16B16A16_UNORM)
			.SetViewDimension(GI::SrvDimension::TEXTURE2D)
			.SetTexture2D_MipLevels(1); 
		
		mGBufferRtvs[i] = GI::RtvUsage(mGBuffers[i]);
		mGBufferRtvs[i]
			.SetFormat(GI::Format::FORMAT_R16G16B16A16_UNORM)
			.SetViewDimension(GI::RtvDimension::TEXTURE2D);
	}

	mMainDepth = infra->CreateMemoryResource(
			GI::MemoryResourceDesc()
			.SetDimension(GI::ResourceDimension::TEXTURE2D)
			.SetWidth(mRenderSize.x())
			.SetHeight(mRenderSize.y())
			.SetDepthOrArraySize(1)
			.SetMipLevels(1)
			.SetFormat(GI::Format::FORMAT_R32G8X24_TYPELESS)
			.SetLayout(GI::TextureLayout::LAYOUT_UNKNOWN)
			.SetFlags(GI::ResourceFlag::ALLOW_DEPTH_STENCIL)
			.SetInitState(GI::ResourceState::STATE_DEPTH_WRITE)
			.SetName("SceneDepthStencil")
			.SetHeapType(GI::HeapType::DEFAULT)); 

	mMainDepthDsv = GI::DsvUsage(mMainDepth);
	mMainDepthDsv
		.SetViewDimension(GI::DsvDimension::TEXTURE2D)
		.SetFormat(GI::Format::FORMAT_D32_FLOAT_S8X24_UINT)
		.SetFlags(GI::DsvFlag::NONE);

	mMainDepthSrv = GI::SrvUsage(mMainDepth);
	mMainDepthSrv
		.SetFormat(GI::Format::FORMAT_R32_FLOAT_X8X24_TYPELESS)
		.SetViewDimension(GI::SrvDimension::TEXTURE2D)
		.SetTexture2D_MipLevels(1); 

	mShadowMask = std::make_unique<RenderTarget>(infra, Vec3u{ mRenderSize.x(), mRenderSize.y(), 1 }, GI::Format::FORMAT_R16_FLOAT, "ShadowMask");

	mLightViewDepth = infra->CreateMemoryResource(
			GI::MemoryResourceDesc()
			.SetDimension(GI::ResourceDimension::TEXTURE2D)
			.SetWidth(mRenderSize.x())
			.SetHeight(mRenderSize.y())
			.SetDepthOrArraySize(1)
			.SetMipLevels(1)
			.SetFormat(GI::Format::FORMAT_R24G8_TYPELESS)
			.SetLayout(GI::TextureLayout::LAYOUT_UNKNOWN)
			.SetFlags(GI::ResourceFlag::ALLOW_DEPTH_STENCIL)
			.SetInitState(GI::ResourceState::STATE_DEPTH_WRITE)
			.SetName("LightViewDepth")
			.SetHeapType(GI::HeapType::DEFAULT)); 

	mLightViewDepthDsv = GI::DsvUsage(mLightViewDepth);
	mLightViewDepthDsv
		.SetViewDimension(GI::DsvDimension::TEXTURE2D)
		.SetFormat(GI::Format::FORMAT_D24_UNORM_S8_UINT)
		.SetFlags(GI::DsvFlag::NONE);

	mLightViewDepthSrv = GI::SrvUsage(mLightViewDepth);
	mLightViewDepthSrv
		.SetFormat(GI::Format::FORMAT_R24_UNORM_X8_TYPELESS)
		.SetViewDimension(GI::SrvDimension::TEXTURE2D)
		.SetTexture2D_MipLevels(1);

	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\monobike_derivative\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\seamless_pbr_texture_metal_01\scene.gltf)", Math::Axis3D_Yp);
	SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\free_1975_porsche_911_930_turbo\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\slum_house\scene.gltf)", Math::Axis3D_Yp);
	//SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(D:\Assets\city_test\scene.gltf)", Math::Axis3D_Yp);

	mTestModel.reset(RenderUtils::FromSceneRawData(mRenderModule->GetGraphicsInfra(), sceneRawData));
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

void WorldRenderer::Render(GI::IGraphicsInfra* infra, const GI::RtvUsage& target)
{
	auto* frameGraph = mRenderModule->GetFrameGraph();
	auto* blackboard = frameGraph->GetBlackboard();

	auto& sunLight = blackboard->Get<DirectionalLight>();

	{
		RENDER_EVENT(infra, InitialResources);

		if (!mQuad->IsGraphicsResourceReady())
		{
			mQuad->CreateAndInitialResource(infra);
		}

		if (!mSphere->IsGraphicsResourceReady())
		{
			mSphere->CreateAndInitialResource(infra);
		}

		if (!mSkyTexture->IsGraphicsResourceReady())
		{
			mSkyTexture->CreateAndInitialResource(infra);

			const auto& srcSize = mSkyTexture->GetResource()->GetSize();
			const Vec3u skyRtSize = { 1024, 1024 * srcSize.y() / srcSize.x(), 1 };
			mPanoramicSkyRt = std::make_unique<RenderTarget>(infra, skyRtSize, GI::Format::FORMAT_R32G32B32A32_FLOAT, "PanoramicSkyRt");

			const std::string& customSkyColor = Utils::FormatString("float4(color.xyz * %.2f, 1)", mSkyLightIntensity);
			RenderUtils::CopyTexture(infra, mPanoramicSkyRt->GetRtv(), Vec2f::Zero(), Vec2f{ skyRtSize.x(), skyRtSize.y() }, mSkyTexture->GetSrv(), mNoMipMapLinearSampler, customSkyColor.c_str());

			auto [irradMap, irradMapSrv] = EnvironmentMap::GenerateIrradianceMap(frameGraph, infra, mPanoramicSkyRt->GetSrv(), 8, 10);
			std::swap(mIrradianceMap, irradMap);
			mIrradianceMapSrv = irradMapSrv;

			auto [filterEnvMap, filterEnvMapSrv] = EnvironmentMap::GeneratePrefilteredEnvironmentMap(frameGraph, infra, mPanoramicSkyRt->GetSrv(), 1024);
			std::swap(mFilteredEnvMap, filterEnvMap);
			mFilteredEnvMapSrv = filterEnvMapSrv;

			RenderUtils::GaussianBlur(infra, mPanoramicSkyRt->GetRtv(), mPanoramicSkyRt->GetSrv(), 2);
		}

		if (!mBRDFIntegrationMap)
		{
			auto [map, srv] = EnvironmentMap::GenerateIntegratedBRDF(frameGraph, infra, 1024);
			std::swap(mBRDFIntegrationMap, map);
			mBRDFIntegrationMapSrv = srv;
		}

		mTestModel->ForEach([&](auto& node)
			{
				if (auto& geo = node.mContent.first)
				{
					if (!geo->IsGraphicsResourceReady())
					{
						geo->CreateAndInitialResource(mRenderModule->GetGraphicsInfra());
					}
				}
				if (auto& mat = node.mContent.second)
				{
					if (!mat->IsGpuResourceReady())
					{
						mat->UpdateGpuResources(mRenderModule->GetGraphicsInfra());
					}
				}
			});
	}

	//////////////////////////////////////////////////////////////////////////

	{
		RENDER_EVENT(infra, LightViewDepth);

		infra->GetRecorder()->AddClearOperation(mLightViewDepthDsv, true, sunLight.mLightViewProj.GetFarPlaneDeviceDepth(), true, 0);

		mTestModel->ForEach([&](const auto& node)
			{
				Geometry* geo = node.mContent.first.get();
				RenderMaterial* mat = node.mContent.second.get();

				if (geo && mat && mat->IsGpuResourceReady())
				{
					RenderGeometryDepthWithMaterial(frameGraph, infra, geo, mat, node.mAbsTransform, mLightViewDepthDsv);
				}
			});
	}

	//////////////////////////////////////////////////////////////////////////

	{
		RENDER_EVENT(infra, GBuffer);

		for (const auto& rt : mGBufferRtvs)
		{
			infra->GetRecorder()->AddClearOperation(rt, { 0.f, 0.f, 0.f, 1.f });
		}
		infra->GetRecorder()->AddClearOperation(mMainDepthDsv, true, blackboard->Get<MainCameraState>().mCameraProj.GetFarPlaneDeviceDepth(), true, 0);

		mTestModel->ForEach([&](const auto& node)
			{
				Geometry* geo = node.mContent.first.get();
				RenderMaterial* mat = node.mContent.second.get();

				if (geo && mat && mat->IsGpuResourceReady())
				{
					RenderGeometryWithMaterial(frameGraph, infra, geo, mat, node.mAbsTransform, mGBufferRtvs, mMainDepthDsv);
				}
			});
	}

	//////////////////////////////////////////////////////////////////////////

	RenderShadowMask(frameGraph, infra, mShadowMask->GetRtv(), mLightViewDepthSrv, mNoMipMapLinearDepthCmpSampler, mMainDepthSrv, mNoMipMapLinearSampler);
	DeferredLighting(frameGraph, infra, target);
	RenderSky(frameGraph, target, mMainDepthDsv);
}

void WorldRenderer::RenderGBufferChannels(GI::IGraphicsInfra* infra, const GI::RtvUsage& target)
{
	const std::pair<i32, const char*> gbufferSemantics[] =
	{
		{ 1, "float4(LinearToSrgb(color.xyz), 1)" },		// BaseColor,
		{ 0, "float4(LinearToSrgb(color.xyz), 1)" },		// Normal,
		{ 2, "float4(LinearToSrgb(color.yyy), 1)" },		// MetalMash,
		{ 0, "float4(LinearToSrgb(1.0 - color.www), 1)" },	// Roughness,
		{ 2, "float4(LinearToSrgb(color.zzz), 1)" },		// Reflection,
	};
	
	const auto& targetSize = target.GetResource()->GetSize();
	const f32 width = f32(targetSize.x()) / Utils::GetArrayLength(gbufferSemantics);
	const f32 height = width / targetSize.x() * targetSize.y();

	for (u32 i = 0; i < Utils::GetArrayLength(gbufferSemantics); ++i)
	{
		const auto& [idx, unary] = gbufferSemantics[i];

		RenderUtils::CopyTexture(infra, 
			target, { i * width, 0.f }, { width, height }, 
			mGBufferSrvs[idx], mNoMipMapLinearSampler, unary);
	}
}

void WorldRenderer::RenderShadowMaskChannel(GI::IGraphicsInfra* infra, const GI::RtvUsage& target)
{
	const auto& targetSize = target.GetResource()->GetSize();
	const f32 width = f32(targetSize.x()) * 0.25f;
	const f32 height = f32(targetSize.y()) * 0.25f;;

	RenderUtils::CopyTexture(infra,
		target, { 0.f, targetSize.y() - height }, { width, height },
		mShadowMask->GetSrv(), mNoMipMapLinearSampler, "float4(LinearToSrgb(color.xxx), 1)");
}

void WorldRenderer::RenderLightViewDepthChannel(GI::IGraphicsInfra* infra, const GI::RtvUsage& target)
{
	const auto& targetSize = target.GetResource()->GetSize();
	const f32 size = f32(targetSize.y()) * 0.25f;

	RenderUtils::CopyTexture(infra,
		target, { 0.f, size }, { size, size },
		mLightViewDepthSrv, mNoMipMapLinearSampler, "float4(LinearToSrgb(pow(color.xxx, 5)), 1)");
}

void WorldRenderer::DeferredLighting(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, const GI::RtvUsage& target)
{
	auto& camState = frameGraph->GetBlackboard()->Get<MainCameraState>();
	const auto& cameraProj = camState.mCameraProj;
	const auto& cameraTrans = camState.mCameraTrans;

	auto& sunLight = frameGraph->GetBlackboard()->Get<DirectionalLight>();

	RENDER_EVENT(infra, DeferredLighting);

	const auto& dsSize = mMainDepth->GetSize();

	auto tmpDepth = infra->CreateMemoryResource(
			GI::MemoryResourceDesc()
			.SetDimension(GI::ResourceDimension::TEXTURE2D)
			.SetWidth(dsSize.x())
			.SetHeight(dsSize.y())
			.SetDepthOrArraySize(dsSize.z())
			.SetMipLevels(1)
			.SetFormat(mMainDepth->GetFormat())
			.SetLayout(GI::TextureLayout::LAYOUT_UNKNOWN)
			.SetFlags(GI::ResourceFlag::ALLOW_DEPTH_STENCIL)
			.SetInitState(GI::ResourceState::STATE_DEPTH_WRITE)
			.SetName("TempDepthRt")
			.SetHeapType(GI::HeapType::DEFAULT));

	auto tmpDepthDsv = GI::DsvUsage(tmpDepth);
	tmpDepthDsv
		.SetViewDimension(GI::DsvDimension::TEXTURE2D)
		.SetFormat(mMainDepthDsv.GetFormat())
		.SetFlags(GI::DsvFlag::NONE);

	infra->GetRecorder()->AddCopyOperation(tmpDepth.get(), mMainDepth.get());

	GI::GraphicsPass lightingPass;

	lightingPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	lightingPass.mRootSignatureDesc.mEntry = "GraphicsRS";
	lightingPass.mVsFile = "res/Shader/Lighting.hlsl";
	lightingPass.mPsFile = "res/Shader/Lighting.hlsl";

	lightingPass.mDepthStencilDesc
		.SetDepthEnable(false)
		.SetStencilEnable(true)
		.SetStencilReadMask(RenderUtils::WorldStencilMask_OpaqueObject)
		.SetStencilWriteMask(0);
	lightingPass.mDepthStencilDesc.FrontFace
		.SetStencilFunc(GI::ComparisonFunction::EQUAL)
		.SetStencilPassOp(GI::StencilOp::KEEP)
		.SetStencilFailOp(GI::StencilOp::KEEP);
	lightingPass.mDepthStencilDesc.BackFace
		.SetStencilFunc(GI::ComparisonFunction::EQUAL)
		.SetStencilPassOp(GI::StencilOp::KEEP)
		.SetStencilFailOp(GI::StencilOp::KEEP);

	lightingPass.mInputLayout = mQuad->mVertexElementDescs;

	const auto& targetSize = target.GetResource()->GetSize();
	lightingPass.SetRtv(0, target);
	lightingPass.SetDsv(tmpDepthDsv);
	lightingPass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
	lightingPass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };
	lightingPass.mStencilRef = RenderUtils::WorldStencilMask_OpaqueObject;

	lightingPass.PushVbv(mQuad->GetVbvDesc());
	lightingPass.SetIbv(mQuad->GetIbvDesc());
	lightingPass.mIndexCount = mQuad->mIndices.size();

	lightingPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
	lightingPass.AddCbVar("FrustumInfo", Vec4f{ cameraProj.GetHalfFovHorizontal(), cameraProj.GetHalfFovVertical(), cameraProj.mNear, cameraProj.mFar });
	lightingPass.AddSrv("GBuffer0", mGBufferSrvs[0]);
	lightingPass.AddSrv("GBuffer1", mGBufferSrvs[1]);
	lightingPass.AddSrv("GBuffer2", mGBufferSrvs[2]);
	lightingPass.AddSrv("ShadowMask", mShadowMask->GetSrv());
	lightingPass.AddSampler("ShadowMaskSampler", mLightingSceneSampler);
	lightingPass.AddSrv("SceneDepth", mMainDepthSrv);
	lightingPass.AddSampler("GBufferSampler", mLightingSceneSampler);

	lightingPass.AddCbVar("InvViewMat", cameraTrans.ComputeInvViewMatrix());
	lightingPass.AddCbVar("InvProjMat", cameraProj.ComputeInvProjectionMatrix());

	lightingPass.AddSrv("PrefilteredEnvMap", mFilteredEnvMapSrv);
	lightingPass.AddSampler("PrefilteredEnvMapSampler", mFilteredEnvMapSampler);
	lightingPass.AddCbVar("PrefilteredInfo", Vec4f{ f32(mFilteredEnvMap->GetMipLevelCount()), 0.f, 0.f, 0.f });
	
	lightingPass.AddSrv("IrradianceMap", mIrradianceMapSrv);
	lightingPass.AddSampler("IrradianceMapSampler", mPanoramicSkySampler);

	lightingPass.AddSrv("BRDFIntegrationMap", mBRDFIntegrationMapSrv);
	lightingPass.AddSampler("BRDFIntegrationMapSampler", mBRDFIntegrationMapSampler);

	lightingPass.AddCbVar("CameraDir", cameraTrans.CamDirInWorldSpace());
	lightingPass.AddCbVar("CameraPos", cameraTrans.CamPosInWorldSpace());
	lightingPass.AddCbVar("LightDir", sunLight.mWorldTransform.CamDirInWorldSpace());
	lightingPass.AddCbVar("LightColor", (sunLight.mLightColor * sunLight.mLightIntensity).eval());

	infra->GetRecorder()->AddGraphicsPass(lightingPass);
}

void WorldRenderer::RenderSky(FrameGraph* frameGraph, const GI::RtvUsage& target, const GI::DsvUsage& depth) const
{
	if (!mPanoramicSkyRt) { return; }

	struct PassData
	{
		GI::VbvUsage geoVertices;
		GI::IbvUsage geoIndices;
		GI::SrvUsage panoramicSky;
		GI::SamplerDesc panoramicSampler;
		GI::RtvUsage target;
		GI::DsvUsage depth;
	};

	auto& camState = frameGraph->GetBlackboard()->Get<MainCameraState>();
	const auto& cameraProj = camState.mCameraProj;
	const auto& cameraTrans = camState.mCameraTrans;

	frameGraph->AddPass<PassData>("RenderSky",
		[this, &target, &depth]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.Read(mQuad->GetVbvDesc());
			data.geoIndices = builder.Read(mQuad->GetIbvDesc());
			data.panoramicSky = builder.Read(mPanoramicSkyRt->GetSrv());
			data.panoramicSampler = builder.Read(mPanoramicSkySampler);
			data.target = builder.Write(target);
			data.depth = builder.Write(depth);
		},
		[
			inputLayout = mQuad->mVertexElementDescs,
			indexCount = mQuad->mIndices.size(),
			targetSize = target.GetResource()->GetSize(),
			camProj = camState.mCameraProj,
			camTrans = camState.mCameraTrans
		]
		(const PassData& data, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, Sky);

			GI::GraphicsPass pass;

			const Transformf& transform = Transformf(UniScalingf(1000.f));

			pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
			pass.mRootSignatureDesc.mEntry = "GraphicsRS";
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

			pass.mInputLayout = inputLayout;

			pass.SetRtv(0, data.target);
			pass.SetDsv(data.depth);
			pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
			pass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };
			pass.mStencilRef = 0;

			pass.PushVbv(data.geoVertices);
			pass.SetIbv(data.geoIndices);
			pass.mIndexCount = indexCount;

			pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
			pass.AddCbVar("FrustumInfo", Vec4f{ camProj.GetHalfFovHorizontal(), camProj.GetHalfFovVertical(), camProj.mNear, camProj.mFar });
			pass.AddCbVar("CameraDir", camTrans.CamDirInWorldSpace());
			pass.AddCbVar("InvViewMat", camTrans.ComputeInvViewMatrix());

			pass.AddSrv("PanoramicSky", data.panoramicSky);
			pass.AddSampler("PanoramicSkySampler", data.panoramicSampler);

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void WorldRenderer::RenderGeometryWithMaterial(FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
	Geometry* geometry, RenderMaterial* material,
	const Transformf& transform,
	const std::array<GI::RtvUsage, 3>& gbufferRtvs, const GI::DsvUsage& depthView)
{
	//RENDER_EVENT(infra, WorldRenderer::RenderGeometryWithMaterial);
	
	auto& camState = frameGraph->GetBlackboard()->Get<MainCameraState>();
	const auto& cameraProj = camState.mCameraProj;
	const auto& cameraTrans = camState.mCameraTrans;

	GI::GraphicsPass gbufferPass;

	gbufferPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	gbufferPass.mRootSignatureDesc.mEntry = "GraphicsRS";
	gbufferPass.mVsFile = "res/Shader/GBufferPBRMat01.hlsl";
	gbufferPass.mPsFile = "res/Shader/GBufferPBRMat01.hlsl";

	gbufferPass.mRasterizerDesc
		.SetCullMode(GI::CullMode::NONE);

	gbufferPass.mDepthStencilDesc
		.SetDepthEnable(true)
		.SetDepthFunc(GI::ToDepthCompareFunc(cameraProj.GetNearerDepthCompare()))
		.SetStencilEnable(true)
		.SetStencilReadMask(RenderUtils::WorldStencilMask_Scene)
		.SetStencilWriteMask(RenderUtils::WorldStencilMask_OpaqueObject);
	gbufferPass.mDepthStencilDesc.FrontFace
		.SetStencilDepthFailOp(GI::StencilOp::KEEP)
		.SetStencilFailOp(GI::StencilOp::KEEP)
		.SetStencilPassOp(GI::StencilOp::REPLACE)
		.SetStencilFunc(GI::ComparisonFunction::ALWAYS);
	gbufferPass.mDepthStencilDesc.BackFace
		.SetStencilDepthFailOp(GI::StencilOp::KEEP)
		.SetStencilFailOp(GI::StencilOp::KEEP)
		.SetStencilPassOp(GI::StencilOp::REPLACE)
		.SetStencilFunc(GI::ComparisonFunction::ALWAYS);

	gbufferPass.mInputLayout = geometry->mVertexElementDescs;

	for (i32 i = 0; i < gbufferRtvs.size(); ++i)
	{
		gbufferPass.SetRtv(i, gbufferRtvs[i]);
	}
	gbufferPass.SetDsv(depthView);

	const auto& targetSize = gbufferRtvs[0].GetResource()->GetSize();
	gbufferPass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
	gbufferPass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };
	gbufferPass.mStencilRef = RenderUtils::WorldStencilMask_OpaqueObject;

	gbufferPass.PushVbv(geometry->GetVbvDesc());
	gbufferPass.SetIbv(geometry->GetIbvDesc());

	gbufferPass.mIndexCount = geometry->mIndices.size();

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
		if (attr.mTexture && attr.mTexture->IsGraphicsResourceReady())
		{
			gbufferPass.mShaderMacros.push_back(GI::ShaderMacro{ paramName + "_USE_MAP", "" });

			auto res = attr.mTexture->GetResource();
			auto srv = GI::SrvUsage(res);
			srv
				.SetFormat(res->GetFormat())
				.SetViewDimension(GI::GetSrvDimension(res->GetDimension()))
				.SetTexture2D_MipLevels(res->GetMipLevelCount());
			gbufferPass.AddSrv((paramName + "Tex").c_str(), srv);
			gbufferPass.AddSampler((paramName + "Sampler").c_str(), attr.mSampler);
		}
		else
		{
			gbufferPass.AddCbVar((paramName + "ConstantValue").c_str(), attr.mConstantValue);
		}
	}

	infra->GetRecorder()->AddGraphicsPass(gbufferPass);
}

void WorldRenderer::RenderGeometryDepthWithMaterial(
	FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
	Geometry* geometry, RenderMaterial* material,
	const Transformf& transform,
	const GI::DsvUsage& depthView)
{
	//RENDER_EVENT(infra, WorldRenderer::RenderGeometryDepthWithMaterial);

	auto& camState = frameGraph->GetBlackboard()->Get<DirectionalLight>();
	const auto& cameraProj = camState.mLightViewProj;
	const auto& cameraTrans = camState.mWorldTransform;

	GI::GraphicsPass pass;

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
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

	pass.mInputLayout = geometry->mVertexElementDescs;

	pass.SetDsv(depthView);

	const auto& targetSize = depthView.GetResource()->GetSize();
	pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
	pass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };
	pass.mStencilRef = RenderUtils::WorldStencilMask_OpaqueObject;

	pass.PushVbv(geometry->GetVbvDesc());
	pass.SetIbv(geometry->GetIbvDesc());
	pass.mIndexCount = geometry->mIndices.size();

	pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

	pass.AddCbVar("worldMat", transform.matrix());
	pass.AddCbVar("viewMat", cameraTrans.ComputeViewMatrix());
	pass.AddCbVar("projMat", cameraProj.ComputeProjectionMatrix());

	const char* paramName = "BaseColorTex";

	const auto& attr = material->mMatAttriSlots[TextureUsage_BaseColor];
	if (attr.mTexture && attr.mTexture->IsGraphicsResourceReady())
	{
		pass.AddSrv(paramName, attr.mTexture->GetSrv());
		pass.AddSampler((std::string(paramName) + "Sampler").c_str(), attr.mSampler);
	}

	infra->GetRecorder()->AddGraphicsPass(pass);
}

void WorldRenderer::RenderShadowMask(FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
	const GI::RtvUsage& shadowMask, 
	const GI::SrvUsage& lightViewDepth, const GI::SamplerDesc& lightViewDepthSampler, 
	const GI::SrvUsage& cameraViewDepth, const GI::SamplerDesc& cameraViewDepthSampler)
{
	auto* blackboard = frameGraph->GetBlackboard();

	auto& camState = blackboard->Get<MainCameraState>();
	const auto& cameraProj = camState.mCameraProj;
	const auto& cameraTrans = camState.mCameraTrans;

	auto& lightView = blackboard->Get<DirectionalLight>();
	const auto& lightViewTrans = lightView.mWorldTransform;
	const auto& lightViewProj = lightView.mLightViewProj;

	RENDER_EVENT(infra, ShadowMask);

	static Geometry* geometry = Geometry::GenerateQuad();
	if (!geometry->IsGraphicsResourceReady())
	{
		geometry->CreateAndInitialResource(infra);
	}

	GI::GraphicsPass pass;

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/ConstructShadowMask.hlsl";
	pass.mPsFile = "res/Shader/ConstructShadowMask.hlsl";

	pass.mDepthStencilDesc
		.SetDepthEnable(false)
		.SetStencilEnable(false);
	
	pass.mInputLayout = geometry->mVertexElementDescs;

	pass.SetRtv(0, shadowMask);

	const auto& targetSize = shadowMask.GetResource()->GetSize();
	pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
	pass.mScissorRect = { 0, 0, i32(targetSize.x()), i32(targetSize.y()) };

	pass.PushVbv(geometry->GetVbvDesc());
	pass.SetIbv(geometry->GetIbvDesc());
	pass.mIndexCount = geometry->mIndices.size();

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

	infra->GetRecorder()->AddGraphicsPass(pass);
}

