#include "RenderPch.h"
#include "WorldRenderer.h"
#include "RenderModule.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12PipelinePass.h"
#include "D3D12/D3D12RenderTarget.h"
#include "D3D12/D3D12ResourceView.h"
#include "D3D12/D3D12Resource.h"
#include "D3D12/D3D12Geometry.h"

WorldRenderer::WorldRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	mQuad = D3D12Geometry::GenerateQuad(mRenderModule->GetDevice());
}

void WorldRenderer::TickFrame(Timer* timer)
{
	mElapsedTime = timer->GetCurrentFrameElapsedSeconds();
}

void WorldRenderer::Render(GraphicsContext* context, IRenderTargetView* target)
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
	lightingPass.mViewPort = { 0, 0, float(targetSize.x()), float(targetSize.y()) };
	lightingPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

	lightingPass.mVbvs.clear();
	lightingPass.mVbvs.push_back(mQuad->mVbv);
	lightingPass.mIbv = mQuad->mIbv;
	lightingPass.mIndexCount = mQuad->mIbv.SizeInBytes / sizeof(u16);

	lightingPass.AddCbVar("time", mElapsedTime);

	lightingPass.Draw();
}
