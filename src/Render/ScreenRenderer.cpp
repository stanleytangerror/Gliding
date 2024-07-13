#include "RenderPch.h"
#include "ScreenRenderer.h"
#include "RenderModule.h"
#include "Geometry.h"

ScreenRenderer::ScreenRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	mQuad.reset(Geometry::GenerateQuad()->CreateAndInitialResource(mRenderModule->GetFrameGraph()));
}

ScreenRenderer::~ScreenRenderer()
{

}

void ScreenRenderer::TickFrame(Timer* timer)
{
	mLastFrameDeltaTimeInSeconds = timer->GetLastFrameDeltaTime();
	mSecondsSinceLaunch = timer->GetCurrentFrameElapsedSeconds();
}

void ScreenRenderer::Render(const FrameGraphResource& sceneHdr, FrameGraphMutableResource& screenRt)
{
	auto exposure = CalcSceneExposure(sceneHdr);
	ToneMapping(sceneHdr, exposure, screenRt);
}

FrameGraphMutableResource ScreenRenderer::CalcSceneExposure(const FrameGraphResource& sceneHdr)
{
	auto frameGraph = mRenderModule->GetFrameGraph();
	
	const i32 histogramSize = 64;
	const i32 stride = sizeof(u32);
	const f32 brightMin = 4.f;
	const f32 brightMax = 65536.f;

	const auto histogramDesc = GI::MemoryResourceDesc::Buffer2(histogramSize * stride, true, true, "BrightnessHistogram");
	auto histogramFg = frameGraph->CreateTransient(histogramDesc);

	struct BrightnessHistogramPassData
	{
		Vec3u sceneHdrSize;
		SrvUsageFuture sceneHdr;
		UavUsageFuture histogram;
	};

	frameGraph->AddPass<BrightnessHistogramPassData>("BrightnessHistogram",
		[&]
		(RenderPassBuilder& builder, BrightnessHistogramPassData& data)
		{
			data.sceneHdr = builder.ReadTex2DSrv(sceneHdr);
			data.sceneHdrSize = frameGraph->GetResourceDesc(sceneHdr).GetSize();
			data.histogram = builder.WriteBufferUav(histogramFg, histogramSize, stride);
		},
		[brightMin, brightMax, histogramSize]
		(const BrightnessHistogramPassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, BrightnessHistogram);

			GI::ComputePass pass;

			pass.mCsFile = "res/Shader/Exposure.hlsl";
			pass.mShaderMacros.push_back(GI::ShaderMacro{ "CONSTRUCT_HISTOGRAM", "1" });

			const Vec3u& size = data.sceneHdrSize;
			pass.AddSrv("SceneHdr", resources.Get(data.sceneHdr));
			pass.AddCbVar("SceneHdrSize", Vec4f{ f32(size.x()), f32(size.y()), 1.f / size.x(), 1.f / size.y() });

			pass.AddUav("SceneBrightnessHistogram", resources.Get(data.histogram));
			pass.AddCbVar("HistogramInfo", Vec4f{ std::log2(brightMin), std::log2(brightMax), f32(histogramSize), 1.f / histogramSize });

			pass.mThreadGroupCounts = { u32(size.x() / 32 + 1), u32(size.y() / 32 + 1), 1 };

			infra->GetRecorder()->AddComputePass(pass);
		});

	struct HistogramReducePassData
	{
		Vec3u sceneHdrSize;
		SrvUsageFuture histogram;
		UavUsageFuture exposureRt;
	};

	const auto exposureDesc = GI::MemoryResourceDesc::RenderTarget2D({ 1, 1 }, GI::Format::FORMAT_R32G32B32A32_FLOAT,
		GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS, "ExposureRt");
	
	static auto exposureFg = mRenderModule->GetFrameGraph()->CreatePermanent(exposureDesc);

	frameGraph->AddPass<HistogramReducePassData>("HistogramReduce",
		[&]
		(RenderPassBuilder& builder, HistogramReducePassData& data)
		{
			data.sceneHdrSize = frameGraph->GetResourceDesc(sceneHdr).GetSize();
			data.histogram = builder.ReadBufferSrv(histogramFg, histogramSize, stride);
			data.exposureRt = builder.ReadWriteTex2DUav(exposureFg);
		},
		[this, brightMin, brightMax, histogramSize]
		(const HistogramReducePassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, HistogramReduce);

			GI::ComputePass pass;

			pass.mCsFile = "res/Shader/Exposure.hlsl";
			pass.mShaderMacros.push_back(GI::ShaderMacro{ "HISTOGRAM_REDUCE", "1" });

			const Vec3u& size = data.sceneHdrSize;
			pass.AddCbVar("SceneHdrSize", Vec4f{ f32(size.x()), f32(size.y()), 1.f / size.x(), 1.f / size.y() });

			pass.AddCbVar("TimeInfo", Vec4f{ mLastFrameDeltaTimeInSeconds, 1.f / mLastFrameDeltaTimeInSeconds, mSecondsSinceLaunch, 0.f });

			pass.AddSrv("SceneBrightnessHistogram", resources.Get(data.histogram));
			pass.AddCbVar("HistogramInfo", Vec4f{ std::log2(brightMin), std::log2(brightMax), f32(histogramSize), 1.f / histogramSize });
			pass.AddCbVar("EyeAdaptInfo", Vec4f{ mEyeAdaptSpeedUp, mEyeAdaptSpeedDown, 0.f, 0.f });

			pass.AddUav("ExposureTexture", resources.Get(data.exposureRt));

			pass.mThreadGroupCounts = { 1, 1, 1 };

			infra->GetRecorder()->AddComputePass(pass);
		});

	return exposureFg;
}

void ScreenRenderer::ToneMapping(const FrameGraphResource& sceneHdr, const FrameGraphResource& exposure, FrameGraphMutableResource& target)
{
	auto frameGraph = mRenderModule->GetFrameGraph();

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		SrvUsageFuture sceneHdr;
		SrvUsageFuture exposure;
		RtvUsageFuture target;
		Vec3u targetSize;
	};

	frameGraph->AddPass<PassData>("ToneMapping",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.ReadVbv(mQuad->GetVb(), mQuad->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(mQuad->GetIb(), mQuad->GetIbvDesc());
			data.sceneHdr = builder.ReadTex2DSrv(sceneHdr);
			data.exposure = builder.ReadTex2DSrv(exposure);
			data.targetSize = frameGraph->GetResourceDesc(target).GetSize();
			data.target = builder.WriteTex2DRtv(target);
		},
		[
			inputLayout = mQuad->mVertexElementDescs,
			indexCount = mQuad->mIndices.size()
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, ToneMapping);

			GI::GraphicsPass ldrScreenPass;

			ldrScreenPass.mVsFile = "res/Shader/ToneMapping.hlsl";
			ldrScreenPass.mPsFile = "res/Shader/ToneMapping.hlsl";

			ldrScreenPass.mDepthStencilDesc.SetDepthEnable(false);
			ldrScreenPass.mDepthStencilDesc.SetStencilEnable(false);

			ldrScreenPass.mInputLayout = inputLayout;

			ldrScreenPass.AddCbVar("RtSize", Vec4f{ f32(data.targetSize.x()), f32(data.targetSize.y()), 1.f / data.targetSize.x(), 1.f / data.targetSize.y() });

			// TODO missing sampler ???
			ldrScreenPass.AddSrv("SceneHdr", resources.Get(data.sceneHdr));
			ldrScreenPass.AddSrv("ExposureTexture", resources.Get(data.exposure));

			ldrScreenPass.AddCbVar("ExposureInfo", Vec4f{ -4.f, 0.f, 0.f, 0.f });

			ldrScreenPass.SetRtv(0, resources.Get(data.target));
			ldrScreenPass.mViewPort.SetWidth(data.targetSize.x()).SetHeight(data.targetSize.y());
			ldrScreenPass.mScissorRect = { 0, 0, i32(data.targetSize.x()), i32(data.targetSize.y()) };

			ldrScreenPass.PushVbv(resources.Get(data.geoVertices));
			ldrScreenPass.SetIbv(resources.Get(data.geoIndices));
			ldrScreenPass.mIndexCount = indexCount;

			infra->GetRecorder()->AddGraphicsPass(ldrScreenPass);
		});
}
