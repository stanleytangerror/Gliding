#include "RenderPch.h"
#include "ScreenRenderer.h"
#include "RenderModule.h"
#include "D3D12Backend/D3D12Headers.h"
#include "D3D12Backend/D3D12Device.h"
#include "D3D12Backend/D3D12PipelinePass.h"
#include "D3D12Backend/D3D12RenderTarget.h"
#include "D3D12Backend/D3D12ResourceView.h"
#include "D3D12Backend/D3D12Resource.h"
#include "D3D12Backend/D3D12CommandContext.h"
#include "D3D12Backend/D3D12Geometry.h"
#include "D3D12Backend/D3D12ResourceView.h"

ScreenRenderer::ScreenRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	mQuad = D3D12Geometry::GenerateQuad(mRenderModule->GetDevice());
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

void ScreenRenderer::Render(D3D12Backend::GraphicsContext* context, D3D12Backend::ShaderResourceView* sceneHdr, D3D12Backend::RenderTargetView* screenRt)
{
	std::unique_ptr<D3D12RenderTarget> exposure = std::make_unique<D3D12RenderTarget>(context->GetDevice(), Vec3i{ 1, 1, 1, }, DXGI_FORMAT_R32G32B32A32_FLOAT, "ExposureRt");

	CalcSceneExposure(context, sceneHdr, exposure->GetUav());
	ToneMapping(context, sceneHdr, exposure->GetSrv(), screenRt);
}

void ScreenRenderer::CalcSceneExposure(D3D12Backend::GraphicsContext* context, D3D12Backend::ShaderResourceView* sceneHdr, D3D12Backend::UnorderedAccessView* exposureRt)
{
	const i32 histogramSize = 64;
	const f32 brightMin = 4.f;
	const f32 brightMax = 65536.f;
	std::unique_ptr<D3D12RenderTarget> histogram = std::make_unique<D3D12RenderTarget>(context->GetDevice(), histogramSize, sizeof(u32), DXGI_FORMAT_UNKNOWN, "BrightnessHistogram");

	{
		RENDER_EVENT(context, BrightnessHistogram);

		D3D12Backend::ComputePass pass(context);

		pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
		pass.mRootSignatureDesc.mEntry = "ComputeRS";
		pass.mCsFile = "res/Shader/Exposure.hlsl";
		pass.mShaderMacros.push_back(D3D12Backend::ShaderMacro{ "CONSTRUCT_HISTOGRAM", "1" });

		const Vec3i& size = sceneHdr->GetResource()->GetSize();
		pass.AddSrv("SceneHdr", sceneHdr);
		pass.AddCbVar("SceneHdrSize", Vec4f{ f32(size.x()), f32(size.y()), 1.f / size.x(), 1.f / size.y() });

		pass.AddUav("SceneBrightnessHistogram", histogram->GetUav());
		pass.AddCbVar("HistogramInfo", Vec4f{ std::log2(brightMin), std::log2(brightMax), f32(histogramSize), 1.f / histogramSize });

		pass.mThreadGroupCounts = { u32(size.x() / 32 + 1), u32(size.y() / 32 + 1), 1 };

		pass.Dispatch();
	}

	{
		RENDER_EVENT(context, HistogramReduce);

		D3D12Backend::ComputePass pass(context);

		pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
		pass.mRootSignatureDesc.mEntry = "ComputeRS";
		pass.mCsFile = "res/Shader/Exposure.hlsl";
		pass.mShaderMacros.push_back(D3D12Backend::ShaderMacro{ "HISTOGRAM_REDUCE", "1" });

		const Vec3i& size = sceneHdr->GetResource()->GetSize();
		pass.AddCbVar("SceneHdrSize", Vec4f{ f32(size.x()), f32(size.y()), 1.f / size.x(), 1.f / size.y() });

		pass.AddCbVar("TimeInfo", Vec4f{ mLastFrameDeltaTimeInSeconds, 1.f / mLastFrameDeltaTimeInSeconds, mSecondsSinceLaunch, 0.f });

		pass.AddSrv("SceneBrightnessHistogram", histogram->GetSrv());
		pass.AddCbVar("HistogramInfo", Vec4f{ std::log2(brightMin), std::log2(brightMax), f32(histogramSize), 1.f / histogramSize });
		pass.AddCbVar("EyeAdaptInfo", Vec4f{ mEyeAdaptSpeedUp, mEyeAdaptSpeedDown, 0.f, 0.f });
		
		pass.AddUav("ExposureTexture", exposureRt);

		pass.mThreadGroupCounts = { 1, 1, 1 };

		pass.Dispatch();
	}
}

void ScreenRenderer::ToneMapping(D3D12Backend::GraphicsContext* context, D3D12Backend::ShaderResourceView* sceneHdr, D3D12Backend::ShaderResourceView* exposure, D3D12Backend::RenderTargetView* target)
{
	RENDER_EVENT(context, ToneMapping);

	D3D12Backend::GraphicsPass ldrScreenPass(context);

	ldrScreenPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	ldrScreenPass.mRootSignatureDesc.mEntry = "GraphicsRS";
	ldrScreenPass.mVsFile = "res/Shader/ToneMapping.hlsl";
	ldrScreenPass.mPsFile = "res/Shader/ToneMapping.hlsl";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = ldrScreenPass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { mQuad->mInputDescs.data(), u32(mQuad->mInputDescs.size()) };
	}

	const Vec3i& targetSize = target->GetResource()->GetSize();
	
	ldrScreenPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

	ldrScreenPass.AddSrv("SceneHdr", sceneHdr);
	ldrScreenPass.AddSrv("ExposureTexture", exposure);

	ldrScreenPass.AddCbVar("ExposureInfo", Vec4f{ -4.f, 0.f, 0.f, 0.f });

	ldrScreenPass.mRts[0] = target;
	ldrScreenPass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	ldrScreenPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	
	ldrScreenPass.mVbvs.clear();
	ldrScreenPass.mVbvs.push_back(mQuad->mVbv);
	ldrScreenPass.mIbv = mQuad->mIbv;
	ldrScreenPass.mIndexCount = mQuad->mIbv.SizeInBytes / sizeof(u16);

	ldrScreenPass.Draw();
}
