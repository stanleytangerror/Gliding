#pragma once

#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12ConstantBuffer.h"

class D3D12Device;
class D3D12ConstantBuffer;

class GraphicsContext
{
public:
	GraphicsContext(D3D12Device* device);

	void		Execute(ID3D12CommandQueue* q, D3D12Fence* fence);
	void		Reset();

	ID3D12GraphicsCommandList*	GetCommandList() const { return mCommandList; }
	RuntimeDescriptorHeap*		GetRuntimeHeap() const { return mRuntimeHeap; }
	D3D12ConstantBuffer*		GetConstantBuffer() const { return mConstantBuffer; }
	D3D12Device*				GetDevice() const { return mDevice; }
	u64							GetPlannedFenceValue() const;

	void						Transition(ID3D12Resource* resource, const D3D12_RESOURCE_STATES srcState, const D3D12_RESOURCE_STATES destState);

private:
	D3D12Device*				mDevice = nullptr;
	ID3D12GraphicsCommandList* mCommandList = nullptr;
	ID3D12CommandAllocator* mCurrentCommandAllocator = nullptr;
	RuntimeDescriptorHeap* mRuntimeHeap = nullptr;
	D3D12ConstantBuffer* mConstantBuffer = nullptr;
};

class ComputeContext
{
public:
	ComputeContext(D3D12Device* device);

	void		Execute(ID3D12CommandQueue* q, D3D12Fence* fence);
	void		Reset();

	ID3D12GraphicsCommandList* GetCommandList() const { return mCommandList; }
	RuntimeDescriptorHeap* GetRuntimeHeap() const { return mRuntimeHeap; }
	D3D12Device*			GetDevice() const { return mDevice; }
	u64							GetPlannedFenceValue() const;

	//void						Transition(ID3D12Resource* resource, const D3D12_RESOURCE_STATES srcState, const D3D12_RESOURCE_STATES destState);

private:
	D3D12Device*				mDevice = nullptr;
	ID3D12GraphicsCommandList* mCommandList = nullptr;
	ID3D12CommandAllocator* mCurrentCommandAllocator = nullptr;
	RuntimeDescriptorHeap* mRuntimeHeap = nullptr;
	D3D12ConstantBuffer* mConstantBuffer = nullptr;
};

