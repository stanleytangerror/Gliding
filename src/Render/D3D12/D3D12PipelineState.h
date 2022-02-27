#pragma once

#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12Device.h"
#include "Common/Math.h"
#include <map>

class D3D12Device;
class D3D12PipelineStateLibrary;

class GraphicsPipelineState
{
public:
	GraphicsPipelineState();

	void SetRootSignature(ID3D12RootSignature* rs);
	void SetVertexShader(const D3D12_SHADER_BYTECODE& code);
	void SetPixelShader(const D3D12_SHADER_BYTECODE& code);
	void SetInputLayout(UINT num, const D3D12_INPUT_ELEMENT_DESC* descs);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& desc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc);
	void SetBlendState(const D3D12_BLEND_DESC& desc);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& Descriptor() { return mDesc; }
	void					Finalize(D3D12PipelineStateLibrary* psLib);
	ID3D12PipelineState* Get() const { return mPso; }

protected:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC	mDesc = {};
	ID3D12PipelineState* mPso = nullptr;
};

class ComputePipelineState
{
public:
	ComputePipelineState();

	void SetRootSignature(ID3D12RootSignature* rs);
	void SetComputeShader(const D3D12_SHADER_BYTECODE& code);

	ID3D12PipelineState* Get() const { return mPso; }
	void					Finalize(D3D12PipelineStateLibrary* psLib);

protected:
	D3D12_COMPUTE_PIPELINE_STATE_DESC mDesc = {};
	ID3D12PipelineState* mPso = nullptr;

};

class D3D12PipelineStateLibrary
{
public:
	D3D12PipelineStateLibrary(D3D12Device* device);

	ID3D12PipelineState* CreateGraphicsPso(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
	ID3D12PipelineState* CreateComputePso(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);
	ID3D12RootSignature* CreateRootSignature(const char* file, const char* entry);

public:
	struct FileEntryKey
	{
		const std::string file;
		const std::string entry;

		bool operator<(const FileEntryKey& other) const
		{
			return file != other.file ? file < other.file : entry < other.entry;
		}
	};

protected:
	D3D12Device* const mDevice = nullptr;

	std::map<u64, ID3D12PipelineState*> mGraphicPsoCache;
	std::map<u64, ID3D12PipelineState*> mComputePsoCache;
	std::map<FileEntryKey, ID3D12RootSignature*> mRsCache;
};
