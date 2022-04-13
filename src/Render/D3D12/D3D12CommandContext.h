#pragma once

#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12ConstantBuffer.h"
#include "D3D12/D3D12GpuQueue.h"
#include "D3D12/D3D12DescriptorHeap.h"

class D3D12Device;
class D3D12GpuQueue;
class D3D12ConstantBuffer;
class RuntimeDescriptorHeap;
class ID3D12Res;

class D3D12CommandContext
{
public:
	D3D12CommandContext(D3D12Device* device, D3D12GpuQueue* gpuQueue);
	virtual ~D3D12CommandContext();

	void		Finalize();
	void		Reset();

	ID3D12GraphicsCommandList* GetCommandList() const { return mCommandList; }
	RuntimeDescriptorHeap* GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const { return mRuntimeDescHeaps[type].get(); }
	D3D12ConstantBuffer* GetConstantBuffer() const { return mConstantBuffer.get(); }
	D3D12Device* GetDevice() const { return mDevice; }
	u64							GetPlannedFenceValue() const;

	void						Transition(ID3D12Resource* resource, const D3D12_RESOURCE_STATES srcState, const D3D12_RESOURCE_STATES destState);

	void						CopyResource(ID3D12Res* dst, ID3D12Res* src);
	void						CopyBuffer2D(ID3D12Res* dst, ID3D12Res* src);

protected:
	std::thread::id	mThisCpuThreadId;
	D3D12Device* mDevice = nullptr;
	D3D12GpuQueue* mGpuQueue = nullptr;

	ID3D12GraphicsCommandList* mCommandList = nullptr;
	ID3D12CommandAllocator* mCommandAllocator = nullptr;

	std::unique_ptr<D3D12ConstantBuffer> mConstantBuffer;
	std::array< std::unique_ptr<RuntimeDescriptorHeap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mRuntimeDescHeaps;
};

class GraphicsContext : public D3D12CommandContext
{
public:
	GraphicsContext(D3D12Device* device, D3D12GpuQueue* gpuQueue);
};

class ComputeContext : public D3D12CommandContext
{
public:
	ComputeContext(D3D12Device* device, D3D12GpuQueue* gpuQueue);
};

