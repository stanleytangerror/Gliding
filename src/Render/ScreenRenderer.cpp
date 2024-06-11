#include "RenderPch.h"
#include "ScreenRenderer.h"
#include "RenderModule.h"
#include "RenderTarget.h"
#include "Geometry.h"

ScreenRenderer::ScreenRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	mQuad.reset(Geometry::GenerateQuad());
}

ScreenRenderer::~ScreenRenderer()
{

}

void ScreenRenderer::TickFrame(Timer* timer)
{
	mLastFrameDeltaTimeInSeconds = timer->GetLastFrameDeltaTime();
	mSecondsSinceLaunch = timer->GetCurrentFrameElapsedSeconds();
}

void ScreenRenderer::Render(GI::IGraphicsInfra* infra, const SrvUsageFuture& sceneHdr, const RtvUsageFuture& screenRt)
{
	if (!mQuad->IsGraphicsResourceReady())
	{
		mQuad->CreateAndInitialResource(infra);
	}

	std::unique_ptr<RenderTarget> exposure = std::make_unique<RenderTarget>(infra, Vec3u{ 1, 1, 1, }, GI::Format::FORMAT_R32G32B32A32_FLOAT, "ExposureRt");
	auto exposureFg = mRenderModule->GetFrameGraph()->Import(exposure->GetResource());

	CalcSceneExposure(infra, sceneHdr, { exposureFg, exposure->GetUavDesc() });
	ToneMapping(infra, sceneHdr, { exposureFg, exposure->GetSrv() }, screenRt);
}

void ScreenRenderer::CalcSceneExposure(GI::IGraphicsInfra* infra, const SrvUsageFuture& sceneHdr, const UavUsageFuture& exposureRt)
{
	auto frameGraph = mRenderModule->GetFrameGraph();
	
	const i32 histogramSize = 64;
	const f32 brightMin = 4.f;
	const f32 brightMax = 65536.f;
	auto histogram = std::make_unique<RenderTarget>(infra, histogramSize, sizeof(u32), GI::Format::FORMAT_UNKNOWN, "BrightnessHistogram");
	auto histogramFg = frameGraph->Import(histogram->GetResource());

	struct BrightnessHistogramPassData
	{
		Vec3u sceneHdrSize;
		SrvUsageFuture sceneHdr;
		UavUsageFuture histogram;
		RtvUsageFuture target;
	};

	frameGraph->AddPass<BrightnessHistogramPassData>("BrightnessHistogram",
		[&]
		(RenderPassBuilder& builder, BrightnessHistogramPassData& data)
		{
			data.sceneHdr = builder.Read(sceneHdr);
			data.sceneHdrSize = frameGraph->GetResourceDesc(sceneHdr.resource).GetSize();
			data.histogram = builder.Write({ histogramFg, histogram->GetUavDesc() });
		},
		[brightMin, brightMax, histogramSize]
		(const BrightnessHistogramPassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, BrightnessHistogram);

			GI::ComputePass pass;

			pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
			pass.mRootSignatureDesc.mEntry = "ComputeRS";
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

	frameGraph->AddPass<HistogramReducePassData>("HistogramReduce",
		[&]
		(RenderPassBuilder& builder, HistogramReducePassData& data)
		{
			data.sceneHdrSize = frameGraph->GetResourceDesc(sceneHdr.resource).GetSize();
			data.histogram = builder.Read({ histogramFg, histogram->GetSrvDesc() });
			data.exposureRt = builder.Write(exposureRt);
		},
		[this, brightMin, brightMax, histogramSize]
		(const HistogramReducePassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, HistogramReduce);

			GI::ComputePass pass;

			pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
			pass.mRootSignatureDesc.mEntry = "ComputeRS";
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
}

void ScreenRenderer::ToneMapping(GI::IGraphicsInfra* infra, const SrvUsageFuture& sceneHdr, const SrvUsageFuture& exposure, const RtvUsageFuture& target)
{
	struct PassData
	{
		GI::VbvUsage geoVertices;
		GI::IbvUsage geoIndices;
		SrvUsageFuture sceneHdr;
		SrvUsageFuture exposure;
		RtvUsageFuture target;
		Vec3u targetSize;
	};

	auto frameGraph = mRenderModule->GetFrameGraph();
	frameGraph->AddPass<PassData>("GaussianBlur1D",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.Read(mQuad->GetVbvDesc());
			data.geoIndices = builder.Read(mQuad->GetIbvDesc());
			data.sceneHdr = builder.Read(sceneHdr);
			data.exposure = builder.Read(exposure);
			data.targetSize = frameGraph->GetResourceDesc(target.resource).GetSize();
			data.target = builder.Write(target);
		},
		[
			inputLayout = mQuad->mVertexElementDescs,
			indexCount = mQuad->mIndices.size()
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, ToneMapping);

			GI::GraphicsPass ldrScreenPass;

			ldrScreenPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
			ldrScreenPass.mRootSignatureDesc.mEntry = "GraphicsRS";
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

			ldrScreenPass.PushVbv(data.geoVertices);
			ldrScreenPass.SetIbv(data.geoIndices);
			ldrScreenPass.mIndexCount = indexCount;

			infra->GetRecorder()->AddGraphicsPass(ldrScreenPass);
		});
}
