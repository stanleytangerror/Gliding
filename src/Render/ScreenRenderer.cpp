#include "RenderPch.h"
#include "ScreenRenderer.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12PipelinePass.h"

ScreenRenderer::ScreenRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{

}

void ScreenRenderer::Initial()
{
	mQuad = D3D12Geometry::GenerateQuad(mRenderModule->GetDevice());
}

void ScreenRenderer::TickFrame(Timer* timer)
{
	mElapsedTime = timer->GetCurrentFrameElapsedSeconds();
}

void ScreenRenderer::Render(GraphicsContext* context)
{
	GraphicsPass ldrScreenPass(context);

	ldrScreenPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	ldrScreenPass.mRootSignatureDesc.mEntry = "GraphicsRS";
	ldrScreenPass.mVsFile = "res/Shader/Grid.hlsl";
	ldrScreenPass.mPsFile = "res/Shader/Grid.hlsl";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = ldrScreenPass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { mQuad->mInputDescs.data(), u32(mQuad->mInputDescs.size()) };
	}

	SwapChainBufferResource* rtRes = mRenderModule->GetDevice()->GetBackBuffer()->GetBuffer();
	ldrScreenPass.mRts[0] = rtRes->GetRtv();
	ldrScreenPass.mViewPort = { 0, 0, float(rtRes->GetSize().x()), float(rtRes->GetSize().y()) };
	ldrScreenPass.mScissorRect = { 0, 0, rtRes->GetSize().x(), rtRes->GetSize().y() };
	
	ldrScreenPass.mVbvs.clear();
	ldrScreenPass.mVbvs.push_back(mQuad->mVbv);
	ldrScreenPass.mIbv = mQuad->mIbv;
	ldrScreenPass.mIndexCount = mQuad->mIbv.SizeInBytes / sizeof(u16);

	ldrScreenPass.AddCbVar("time", mElapsedTime);

	ldrScreenPass.Draw();
}
