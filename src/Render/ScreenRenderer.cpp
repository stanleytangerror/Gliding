#include "RenderPch.h"
#include "ScreenRenderer.h"
#include "RenderModule.h"
#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12PipelinePass.h"
#include "D3D12/D3D12RenderTarget.h"
#include "D3D12/D3D12ResourceView.h"
#include "D3D12/D3D12Resource.h"
#include "D3D12/D3D12CommandContext.h"
#include "D3D12/D3D12Geometry.h"
#include "D3D12/D3D12ResourceView.h"

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
	
}

void ScreenRenderer::Render(GraphicsContext* context, IShaderResourceView* input, IRenderTargetView* target)
{
	GraphicsPass ldrScreenPass(context);

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

	ldrScreenPass.AddSrv("SceneHdr", input);

	ldrScreenPass.mRts[0] = target;
	ldrScreenPass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	ldrScreenPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	
	ldrScreenPass.mVbvs.clear();
	ldrScreenPass.mVbvs.push_back(mQuad->mVbv);
	ldrScreenPass.mIbv = mQuad->mIbv;
	ldrScreenPass.mIndexCount = mQuad->mIbv.SizeInBytes / sizeof(u16);

	ldrScreenPass.Draw();
}
