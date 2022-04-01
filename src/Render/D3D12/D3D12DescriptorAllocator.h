#pragma once

#include "Common/FreeList.h"
#include "Render/SuspendedRelease.h"
#include "D3D12Headers.h"
#include <vector>

class D3D12DescriptorBlock
{
public:
	using DescHandleIndex = FreeList<CD3DX12_CPU_DESCRIPTOR_HANDLE>::Index;

public:
	D3D12DescriptorBlock(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const i32 numDescriptors);
	virtual								~D3D12DescriptorBlock();

	DescHandleIndex						AllocCpuDesc();
	void								ReleaseCpuDesc(u64 fenceValue, const DescHandleIndex& index);

	bool								CanAlloc() const { return mAllocator.CanAlloc(); }
	bool								AllFreed() const { return mAllocator.AllFreed(); }

	CD3DX12_CPU_DESCRIPTOR_HANDLE		GetDesc(const DescHandleIndex& index) const;

protected:
	const D3D12_DESCRIPTOR_HEAP_DESC	mDesc;
	ID3D12DescriptorHeap* mDescriptorHeap = nullptr; // ownership
	const i32							mDescriptorSize = 0;
	const i32							mDescriptorNum = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE			mCpuBase = {};

	FreeList<CD3DX12_CPU_DESCRIPTOR_HANDLE>	mAllocator;
};

struct CpuDescItem
{
	D3D12DescriptorBlock* mBlock = nullptr;
	FreeList<CD3DX12_CPU_DESCRIPTOR_HANDLE>::Index mIndex = FreeList<CD3DX12_CPU_DESCRIPTOR_HANDLE>::InvalidIndex;

	CD3DX12_CPU_DESCRIPTOR_HANDLE		Get() const;
};

class D3D12DescriptorAllocator
{
public:
	D3D12DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);
	virtual										~D3D12DescriptorAllocator();

	void										UpdateCompletedFenceValue(u64 val);
	CpuDescItem									AllocCpuDesc();
	void										ReleaseCpuDesc(u64 fenceValue, const CpuDescItem& item);

protected:
	static const u32							msDescriptorBlockSize = 1024;
	
	const D3D12_DESCRIPTOR_HEAP_TYPE			mDescriptorType = {};
	ID3D12Device*								mDevice = nullptr;

	std::vector<D3D12DescriptorBlock*>			mActiveBlocks;
	SuspendedReleasePool<D3D12DescriptorBlock>	mBlockPool;
};

