#include "RenderPch.h"
#include "ScreenRenderer.h"
#include "RenderModule.h"
#include "RenderTarget.h"
#include "Geometry.h"

ScreenRenderer::ScreenRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	mQuad = Geometry::GenerateQuad();
}

ScreenRenderer::~ScreenRenderer()
{
	Utils::SafeDelete(mQuad);
}

void ScreenRenderer::TickFrame(Timer* timer)
{
	mLastFrameDeltaTimeInSeconds = timer->GetLastFrameDeltaTime();
	mSecondsSinceLaunch = timer->GetCurrentFrameElapsedSeconds();
}

void ScreenRenderer::Render(GI::IGraphicsInfra* infra, const GI::SrvDesc& sceneHdr, const GI::RtvDesc& screenRt)
{
	if (!mQuad->IsGraphicsResourceReady())
	{
		mQuad->CreateAndInitialResource(infra);
	}

	std::unique_ptr<RenderTarget> exposure = std::make_unique<RenderTarget>(infra, Vec3i{ 1, 1, 1, }, GI::Format::FORMAT_R32G32B32A32_FLOAT, "ExposureRt");

	CalcSceneExposure(infra, sceneHdr, exposure->GetUav());
	ToneMapping(infra, sceneHdr, exposure->GetSrv(), screenRt);
}

void ScreenRenderer::CalcSceneExposure(GI::IGraphicsInfra* infra, const GI::SrvDesc& sceneHdr, const GI::UavDesc& exposureRt)
{
	const i32 histogramSize = 64;
	const f32 brightMin = 4.f;
	const f32 brightMax = 65536.f;
	auto histogram = std::make_unique<RenderTarget>(infra, histogramSize, sizeof(u32), GI::Format::FORMAT_UNKNOWN, "BrightnessHistogram");

	{
		RENDER_EVENT(infra, BrightnessHistogram);

		GI::ComputePass pass;

		pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
		pass.mRootSignatureDesc.mEntry = "ComputeRS";
		pass.mCsFile = "res/Shader/Exposure.hlsl";
		pass.mShaderMacros.push_back(GI::ShaderMacro{ "CONSTRUCT_HISTOGRAM", "1" });

		const Vec3i& size = sceneHdr.GetResource()->GetDimSize();
		pass.AddSrv("SceneHdr", sceneHdr);
		pass.AddCbVar("SceneHdrSize", Vec4f{ f32(size.x()), f32(size.y()), 1.f / size.x(), 1.f / size.y() });

		pass.AddUav("SceneBrightnessHistogram", histogram->GetUav());
		pass.AddCbVar("HistogramInfo", Vec4f{ std::log2(brightMin), std::log2(brightMax), f32(histogramSize), 1.f / histogramSize });

		pass.mThreadGroupCounts = { u32(size.x() / 32 + 1), u32(size.y() / 32 + 1), 1 };

		infra->GetRecorder()->AddComputePass(pass);
	}

	{
		RENDER_EVENT(infra, HistogramReduce);

		GI::ComputePass pass;

		pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
		pass.mRootSignatureDesc.mEntry = "ComputeRS";
		pass.mCsFile = "res/Shader/Exposure.hlsl";
		pass.mShaderMacros.push_back(GI::ShaderMacro{ "HISTOGRAM_REDUCE", "1" });

		const Vec3i& size = sceneHdr.GetResource()->GetDimSize();
		pass.AddCbVar("SceneHdrSize", Vec4f{ f32(size.x()), f32(size.y()), 1.f / size.x(), 1.f / size.y() });

		pass.AddCbVar("TimeInfo", Vec4f{ mLastFrameDeltaTimeInSeconds, 1.f / mLastFrameDeltaTimeInSeconds, mSecondsSinceLaunch, 0.f });

		pass.AddSrv("SceneBrightnessHistogram", histogram->GetSrv());
		pass.AddCbVar("HistogramInfo", Vec4f{ std::log2(brightMin), std::log2(brightMax), f32(histogramSize), 1.f / histogramSize });
		pass.AddCbVar("EyeAdaptInfo", Vec4f{ mEyeAdaptSpeedUp, mEyeAdaptSpeedDown, 0.f, 0.f });
		
		pass.AddUav("ExposureTexture", exposureRt);

		pass.mThreadGroupCounts = { 1, 1, 1 };

		infra->GetRecorder()->AddComputePass(pass);
	}
}

void ScreenRenderer::ToneMapping(GI::IGraphicsInfra* infra, const GI::SrvDesc& sceneHdr, const GI::SrvDesc& exposure, const GI::RtvDesc& target)
{
	RENDER_EVENT(infra, ToneMapping);

	GI::GraphicsPass ldrScreenPass;

	ldrScreenPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	ldrScreenPass.mRootSignatureDesc.mEntry = "GraphicsRS";
	ldrScreenPass.mVsFile = "res/Shader/ToneMapping.hlsl";
	ldrScreenPass.mPsFile = "res/Shader/ToneMapping.hlsl";

	ldrScreenPass.mDepthStencilDesc.SetDepthEnable(false);
	ldrScreenPass.mDepthStencilDesc.SetStencilEnable(false);

	ldrScreenPass.mInputLayout = mQuad->mVertexElementDescs;

	const Vec3i& targetSize = target.GetResource()->GetDimSize();
	
	ldrScreenPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

	ldrScreenPass.AddSrv("SceneHdr", sceneHdr);
	ldrScreenPass.AddSrv("ExposureTexture", exposure);

	ldrScreenPass.AddCbVar("ExposureInfo", Vec4f{ -4.f, 0.f, 0.f, 0.f });

	ldrScreenPass.mRtvs[0] = target;
	ldrScreenPass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
	ldrScreenPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	
	ldrScreenPass.mVbvs.clear();
	ldrScreenPass.mVbvs.push_back(mQuad->GetVbvDesc());
	ldrScreenPass.mIbv = mQuad->GetIbvDesc();
	ldrScreenPass.mIndexCount = mQuad->mIndices.size();

	infra->GetRecorder()->AddGraphicsPass(ldrScreenPass);
}
