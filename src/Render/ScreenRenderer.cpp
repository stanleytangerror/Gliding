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
	std::vector<Vec2f> vertices =
	{
		Vec2f{ 1.f, -1.f },Vec2f{ 1.f, 1.f },Vec2f{ -1.f, 1.f },Vec2f{ -1.f, -1.f }
	};
	mVb = D3D12Utils::CreateUploadBuffer(mRenderModule->GetDevice()->GetDevice(), vertices.size() * sizeof(Vec2f));

	std::vector<u16> indices = { 0, 1, 2, 0, 2, 3 };
	mIb = D3D12Utils::CreateUploadBuffer(mRenderModule->GetDevice()->GetDevice(), indices.size() * sizeof(u16));

	{
		u8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		AssertHResultOk(mVb->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, vertices.data(), vertices.size() * sizeof(Vec2f));
	}

	{
		u8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		AssertHResultOk(mIb->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices.data(), indices.size() * sizeof(u16));
	}

	mInputDescs =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mVbv.BufferLocation = mVb->GetGPUVirtualAddress();
	mVbv.StrideInBytes = sizeof(Vec2f);
	mVbv.SizeInBytes = vertices.size() * sizeof(Vec2f);

	mIbv.BufferLocation = mIb->GetGPUVirtualAddress();
	mIbv.SizeInBytes = indices.size() * sizeof(u16);
	mIbv.Format = DXGI_FORMAT_R16_UINT;
}

void ScreenRenderer::Render()
{
	//// generate ldr screen tex
	//pass.Write(mLdrScreenTex);
	//pass.SetComputeShader(cs);
	//pass.SetConstBuffer();
	//pass.Dispatch();

	//// draw ldr screen tex to swapchain
	GraphicsPass ldrScreenPass(mRenderModule->GetDevice()->GetGraphicContextPool()->AllocItem());

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
	}

	ldrScreenPass.mRts[0] = mRenderModule->GetDevice()->GetBackBuffer()->GetBuffer()->GetRtv();

	ldrScreenPass.mVbvs.clear();
	ldrScreenPass.mVbvs.push_back(mVbv);
	ldrScreenPass.mIbv = mIbv;
	ldrScreenPass.mIndexCount = mIbv.SizeInBytes / sizeof(u16);

	ldrScreenPass.Draw();

	//pass.Read(mLdrScreenTex);
	//pass.Write(swapchain);
	//pass.SetComputeShader(cs);
	//pass.SetConstBuffer();
	//pass.Dispatch();
}
