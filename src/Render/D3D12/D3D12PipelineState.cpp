#include "RenderPch.h"
#include "D3D12PipelineState.h"
#include "D3D12/D3D12Utils.h"

GraphicsPipelineState::GraphicsPipelineState()
{
	mDesc.InputLayout = {};
	mDesc.pRootSignature = nullptr;
	mDesc.VS = {};
	mDesc.PS = {};

	mDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	mDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	mDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	mDesc.SampleMask = UINT_MAX;
	mDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	mDesc.NumRenderTargets = 1;
	for (auto& format : mDesc.RTVFormats)
	{
		format = DXGI_FORMAT_UNKNOWN;
	}

	mDesc.DSVFormat = DXGI_FORMAT_D16_UNORM;
	mDesc.SampleDesc.Count = 1;
}

void GraphicsPipelineState::SetRootSignature(ID3D12RootSignature* rs)
{
	mDesc.pRootSignature = rs;
}

void GraphicsPipelineState::SetVertexShader(const D3D12_SHADER_BYTECODE& code)
{
	mDesc.VS = code;
}

void GraphicsPipelineState::SetPixelShader(const D3D12_SHADER_BYTECODE& code)
{
	mDesc.PS = code;
}

void GraphicsPipelineState::SetInputLayout(UINT num, const D3D12_INPUT_ELEMENT_DESC* descs)
{
	mDesc.InputLayout = { descs, num };
}

void GraphicsPipelineState::SetRasterizerState(const D3D12_RASTERIZER_DESC& desc)
{
	mDesc.RasterizerState = desc;
}

void GraphicsPipelineState::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc)
{
	mDesc.DepthStencilState = desc;
}

void GraphicsPipelineState::SetBlendState(const D3D12_BLEND_DESC& desc)
{
	mDesc.BlendState = desc;
}

void GraphicsPipelineState::Finalize(D3D12PipelineStateLibrary* psLib)
{
	mPso = psLib->CreateGraphicsPso(mDesc);
}

ComputePipelineState::ComputePipelineState()
{
	mDesc = {};
}

void ComputePipelineState::SetRootSignature(ID3D12RootSignature* rs)
{
	mDesc.pRootSignature = rs;
}

void ComputePipelineState::SetComputeShader(const D3D12_SHADER_BYTECODE& code)
{
	mDesc.CS = code;
}

void ComputePipelineState::Finalize(D3D12PipelineStateLibrary* psLib)
{
	mPso = psLib->CreateComputePso(mDesc);
}

namespace
{
	template <typename PSO_DESC>
	u64 HashPsoDesc(const PSO_DESC& desc)
	{
		u64 h = 0;
		const byte* const buffer = reinterpret_cast<const byte*>(&desc);
		for (i32 i = 0; i < sizeof(PSO_DESC); ++i)
		{
			h ^= std::hash<byte>{}(buffer[i]) << 1;
		}
		return h;
	}
}

D3D12PipelineStateLibrary::D3D12PipelineStateLibrary(D3D12Device* device)
	: mDevice(device)
{

}

ID3D12PipelineState* D3D12PipelineStateLibrary::CreateGraphicsPso(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	const uint64_t hash = HashPsoDesc(desc);

	if (mGraphicPsoCache.find(hash) == mGraphicPsoCache.end())
	{
		ID3D12PipelineState* pso = nullptr;
		AssertHResultOk(mDevice->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));
		mGraphicPsoCache[hash] = pso;
	}

	return mGraphicPsoCache[hash];
}

ID3D12PipelineState* D3D12PipelineStateLibrary::CreateComputePso(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
{
	const uint64_t hash = HashPsoDesc(desc);

	if (mComputePsoCache.find(hash) == mComputePsoCache.end())
	{
		ID3D12PipelineState* pso = nullptr;
		AssertHResultOk(mDevice->GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso)));
		mComputePsoCache[hash] = pso;
	}

	return mComputePsoCache[hash];
}

ID3D12RootSignature* D3D12PipelineStateLibrary::CreateRootSignature(const wchar_t* file, const char* entry)
{
	Assert(file);
	Assert(entry);

	FileEntryKey key = { file, entry };
	if (mRsCache.find(key) == mRsCache.end())
	{
		ID3DBlob* signature = D3D12Utils::LoadRs(file, entry);
		ID3D12RootSignature* rootSignature = nullptr;
		AssertHResultOk(mDevice->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
		Assert(rootSignature);
		mRsCache[key] = rootSignature;
	}

	return mRsCache[key];
}


