#include "RenderPch.h"
#include "RenderUtils.h"
#include "D3D12/D3D12PipelinePass.h"
#include "D3D12/D3D12Geometry.h"
#include "D3D12/D3D12CommandContext.h"
#include "D3D12/D3D12ResourceView.h"
#include "D3D12/D3D12Resource.h"
#include "D3D12/D3D12RenderTarget.h"

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

void GaussianBlur1D(GraphicsContext* context, IRenderTargetView* target, IShaderResourceView* source, i32 kernelSizeInPixel, D3D12SamplerView* sampler, D3D12Geometry* quad, bool isHorizontal)
{
	auto NormalDistPdf = [](f32 x, f32 stdDev) { return exp(-0.5f * (x * x / stdDev / stdDev) / stdDev) / Math::Sqrt(2.f * Math::Pi<f32>()); };

	const auto& size = source->GetResource()->GetSize();
	const auto& weight4fSize = (kernelSizeInPixel + 1 + 3) / 4;

	GraphicsPass pass(context);

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/GaussianBlur.hlsl";
	pass.mPsFile = "res/Shader/GaussianBlur.hlsl";
	pass.mShaderMacros.push_back(ShaderMacro{ "WEIGHT_SIZE", Utils::FormatString("%d", weight4fSize) });
	pass.mShaderMacros.push_back(ShaderMacro{ isHorizontal ? "HORIZONTAL" : "VERTICAL", "1"});

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { quad->mInputDescs.data(), u32(quad->mInputDescs.size()) };
	}

	pass.mRts[0] = target;
	pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, f32(size.x()), f32(size.y()));
	pass.mScissorRect = { 0, 0, size.x(), size.y() };

	pass.mVbvs.clear();
	pass.mVbvs.push_back(quad->mVbv);
	pass.mIbv = quad->mIbv;
	pass.mIndexCount = quad->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("BlurTargetSize", Vec4f{ f32(size.x()), f32(size.y()), 1.f / size.x(), 1.f / size.y() });
	pass.AddSrv("SourceTex", source);
	pass.AddSampler("SourceTexSampler", sampler);

	std::vector<f32> weights;
	f32 totalWeight = 0.f;
	for (i32 i = 0; i < weight4fSize * 4; ++i)
	{
		f32 w = NormalDistPdf(i, kernelSizeInPixel * 0.5f);
		weights.push_back(w);
		totalWeight += (i == 0) ? w : w * 2;
	}
	for (auto& w : weights)
	{
		w /= totalWeight;
	}
	pass.AddCbVar("Weights", weights);

	pass.Draw();
}

void RenderUtils::GaussianBlur(GraphicsContext* context, IRenderTargetView* target, IShaderResourceView* source, i32 kernelSizeInPixel)
{
	static D3D12SamplerView* sampler = new D3D12SamplerView(context->GetDevice(), D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	static D3D12Geometry* quad = D3D12Geometry::GenerateQuad(context->GetDevice());

	std::unique_ptr<D3D12RenderTarget> interRt = std::make_unique<D3D12RenderTarget>(context->GetDevice(), source->GetResource()->GetSize(), source->GetFormat(), "GaussianBlurIntermediateRT");

	RENDER_EVENT(context, GaussianBlur);
	GaussianBlur1D(context, interRt->GetRtv(), source, kernelSizeInPixel, sampler, quad, true);
	GaussianBlur1D(context, target, interRt->GetSrv(), kernelSizeInPixel, sampler, quad, false);
}
