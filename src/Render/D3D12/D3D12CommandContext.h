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

class D3D12CommandContext
{
public:
	D3D12CommandContext(D3D12Device* device, D3D12GpuQueue* gpuQueue);

	void		Finalize();
	void		Reset();

	ID3D12GraphicsCommandList* GetCommandList() const { return mCommandList; }
	RuntimeDescriptorHeap* GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const { return mRuntimeDescHeaps[type]; }
	D3D12ConstantBuffer* GetConstantBuffer() const { return mConstantBuffer; }
	D3D12Device* GetDevice() const { return mDevice; }
	u64							GetPlannedFenceValue() const;

	void						Transition(ID3D12Resource* resource, const D3D12_RESOURCE_STATES srcState, const D3D12_RESOURCE_STATES destState);

protected:
	std::thread::id	mThisCpuThreadId;
	D3D12Device* mDevice = nullptr;
	D3D12GpuQueue* mGpuQueue = nullptr;

	ID3D12GraphicsCommandList* mCommandList = nullptr;
	ID3D12CommandAllocator* mCommandAllocator = nullptr;

	D3D12ConstantBuffer* mConstantBuffer = nullptr;
	std::array<RuntimeDescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mRuntimeDescHeaps = {};
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

