#pragma once

#include "D3D12Headers.h"
#include "D3D12GpuQueue.h"
#include "D3D12DescriptorAllocator.h"
#include "D3D12PipelineState.h"
#include "D3D12Shader.h"
#include "D3D12ResourceManager.h"
#include "PresentPort.h"

class D3D12PipelineStateLibrary;
class D3D12ShaderLibrary;
class D3D12DescriptorAllocator;
class SwapChainBuffers;
class D3D12GpuQueue;
class D3D12ResourceManager;
enum D3D12GpuQueueType : u8;

class D3D12Device
{
public:
	D3D12Device();

	void	CreateSwapChain(PresentPortType type, HWND windowHandle, const Vec2i& initWindowSize);
	void	StartFrame();
	void	Present();
	void	Destroy();

	ID3D12Device* GetDevice() const;

public:
	D3D12DescriptorAllocator* GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
	D3D12ShaderLibrary* GetShaderLib() const { return mShaderLib; }
	D3D12PipelineStateLibrary* GetPipelineStateLib() const { return mPipelineStateLib; }
	CpuDescItem	GetNullSrvUavCbvCpuDesc() const { return mNullSrvCpuDesc; }
	CpuDescItem	GetNullSamplerCpuDesc() const { return mNullSamplerCpuDesc; }
	PresentPort GetPresentPort(PresentPortType type) const;

	D3D12GpuQueue* GetGpuQueue(D3D12GpuQueueType type) const { return mGpuQueues[u64(type)]; }

	void	ReleaseD3D12Resource(ID3D12Resource*& res);

private:
	IDXGIFactory4* mFactory = nullptr;
	ID3D12Device* mDevice = nullptr;

	std::map<PresentPortType, PresentPort>	mPresentPorts;

	std::array<D3D12GpuQueue*, u64(D3D12GpuQueueType::Count)>	mGpuQueues;
	std::unique_ptr<D3D12ResourceManager>			mResMgr;

	D3D12PipelineStateLibrary* mPipelineStateLib = nullptr;
	D3D12ShaderLibrary* mShaderLib = nullptr;

	std::array<D3D12DescriptorAllocator*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mDescAllocator = {};
	CpuDescItem	mNullSrvCpuDesc;
	CpuDescItem	mNullSamplerCpuDesc;

	static const u32 mSwapChainBufferCount = 3;
};