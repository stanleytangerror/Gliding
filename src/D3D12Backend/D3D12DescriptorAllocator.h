#pragma once

#include "D3D12Headers.h"
#include "D3D12DescriptorHeap.h"
#include <vector>

namespace D3D12Backend
{
	class DescriptorPool
	{
	public:
		using DescHandleIndex = FreeList<CD3DX12_CPU_DESCRIPTOR_HANDLE>::Index;

	public:
		DescriptorPool(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const i32 numDescriptors);

		DescHandleIndex						AllocCpuDesc();
		void								ReleaseCpuDesc(u64 fenceValue, const DescHandleIndex& index);

		bool								CanAlloc() const { return mAllocator.CanAlloc(); }
		bool								AllFreed() const { return mAllocator.AllFreed(); }

		CD3DX12_CPU_DESCRIPTOR_HANDLE		GetDesc(const DescHandleIndex& index) const;

	protected:
		std::unique_ptr<DescriptorArrayResource>		mDescriptorMemory;

		FreeList<CD3DX12_CPU_DESCRIPTOR_HANDLE>	mAllocator;
	};

	struct DescriptorPtr
	{
		DescriptorPool* mBlock = nullptr;
		FreeList<CD3DX12_CPU_DESCRIPTOR_HANDLE>::Index mIndex = FreeList<CD3DX12_CPU_DESCRIPTOR_HANDLE>::InvalidIndex;

		CD3DX12_CPU_DESCRIPTOR_HANDLE		Get() const;
	};

	class D3D12DescriptorAllocator
	{
	public:
		D3D12DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);
		virtual										~D3D12DescriptorAllocator();

		void										UpdateCompletedFenceValue(u64 val);
		DescriptorPtr									AllocCpuDesc();
		void										ReleaseCpuDesc(u64 fenceValue, const DescriptorPtr& item);

	protected:
		static const u32							msDescriptorBlockSize = 1024;

		const D3D12_DESCRIPTOR_HEAP_TYPE			mDescriptorType = {};
		ID3D12Device* mDevice = nullptr;

		std::vector<DescriptorPool*>			mActiveBlocks;
		SuspendedReleasePool<DescriptorPool>	mBlockPool;
	};

}