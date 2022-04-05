#include "RenderPch.h"
#include "RenderUtils.h"
#include "D3D12/D3D12PipelinePass.h"
#include "D3D12/D3D12Geometry.h"
#include "D3D12/D3D12CommandContext.h"
#include "D3D12/D3D12ResourceView.h"
#include "D3D12/D3D12Resource.h"

void RenderUtils::CopyTexture(GraphicsContext* context, 
	IRenderTargetView* target, const Vec2f& targetOffset, const Vec2f& targetRect, 
	IShaderResourceView* source, D3D12SamplerView* sourceSampler, const char* sourcePixelUnary)
{
	static D3D12Geometry* quad = D3D12Geometry::GenerateQuad(context->GetDevice());

	GraphicsPass pass(context);

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/CopyTexture.hlsl";
	pass.mPsFile = "res/Shader/CopyTexture.hlsl";
	pass.mShaderMacros.push_back(ShaderMacro{ "SOURCE_PIXEL_UNARY", sourcePixelUnary ? sourcePixelUnary : "color"});

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { quad->mInputDescs.data(), u32(quad->mInputDescs.size()) };
	}

	const Vec3i& targetSize = target->GetResource()->GetSize();
	pass.mRts[0] = target;
	pass.mViewPort = CD3DX12_VIEWPORT(targetOffset.x(), targetOffset.y(), targetRect.x(), targetRect.y());
	pass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

	pass.mVbvs.clear();
	pass.mVbvs.push_back(quad->mVbv);
	pass.mIbv = quad->mIbv;
	pass.mIndexCount = quad->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("RtSize", Vec4f{ targetRect.x(), targetRect.y(), 1.f / targetRect.x(), 1.f / targetRect.y() });
	pass.AddSrv("SourceTex", source);
	pass.AddSampler("SourceTexSampler", sourceSampler);

	pass.Draw();
}
