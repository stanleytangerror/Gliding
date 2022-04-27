#pragma once

#include "Common/Pool.h"
#include "D3D12Headers.h"

class DescriptorHeapBlock
{
public:
	DescriptorHeapBlock(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const bool shaderVisible, const i32 numDescriptors);
	virtual								~DescriptorHeapBlock();

	CD3DX12_CPU_DESCRIPTOR_HANDLE		GetCpuBaseWithOffset(i32 offset) const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE		GetGpuBaseWithOffset(i32 offset) const;
	ID3D12DescriptorHeap*				GetDescriptorHeap() const { return mDescriptorHeap; }
	i32									GetNumDescriptos() const { return mDescriptorNum; }

protected:
	const D3D12_DESCRIPTOR_HEAP_DESC	mDesc;
	ID3D12DescriptorHeap*				mDescriptorHeap = nullptr; // ownership
	const i32							mDescriptorSize = 0;
	const i32							mDescriptorNum = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE			mCpuBase = {};
	D3D12_GPU_DESCRIPTOR_HANDLE			mGpuBase = {};
};

class RuntimeDescriptorHeap
{
public:
	RuntimeDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);
	virtual								~RuntimeDescriptorHeap();

	CD3DX12_GPU_DESCRIPTOR_HANDLE		Push(const i32 handleCount, const D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescHandles);

	ID3D12DescriptorHeap*				GetCurrentDescriptorHeap() const { return mCurrentWorkingBlock ? mCurrentWorkingBlock->GetDescriptorHeap() : nullptr; }

	u32									GetDescHandleSize() const;

	void								Reset();

protected:
	static const i32					msNumDescriptorsPerBlock = 32;
	ID3D12Device*						mDevice = nullptr;

	const D3D12_DESCRIPTOR_HEAP_TYPE	mDescriptorType = {};
	Pool<DescriptorHeapBlock>			mPool;

	DescriptorHeapBlock*				mCurrentWorkingBlock = nullptr;
	i32									mCurrentWorkingIndex = 0;
};