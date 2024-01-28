#include "D3D12BackendPch.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12DescriptorAllocator.h"
#include "D3D12Utils.h"

namespace D3D12Backend
{
	//////////////////////////////////////////////////////////////////////////

	DescriptorPool::DescriptorPool(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const i32 numDescriptors)
		: mDescriptorMemory(new DescriptorArrayResource(device, type, false, numDescriptors, "DescriptorPool"))
		, mAllocator(numDescriptors)
	{
	}

	DescriptorPool::DescHandleIndex DescriptorPool::AllocCpuDesc()
	{
		return mAllocator.Pop();
	}

	void DescriptorPool::ReleaseCpuDesc(u64 fenceValue, const DescHandleIndex& index)
	{
		mAllocator.Push(index);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorPool::GetDesc(const DescHandleIndex& index) const
	{
		return mDescriptorMemory->GetCpuBaseWithOffset(index);
	}

	//////////////////////////////////////////////////////////////////////////

	CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorPtr::Get() const
	{
		return mBlock ? mBlock->GetDesc(mIndex) : CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());
	}


	DescriptorPtr::operator bool() const
	{
		return mBlock != nullptr;
	}

	//////////////////////////////////////////////////////////////////////////

	D3D12DescriptorAllocator::D3D12DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type)
		: mDescriptorType(type)
		, mDevice(device)
		, mBlockPool(
			[device, type]() { return new DescriptorPool(device, type, msDescriptorBlockSize); },
			[](DescriptorPool* t) {},
			[](DescriptorPool* t) { if (t) { delete t; } }
		)
	{
	}

	D3D12DescriptorAllocator::~D3D12DescriptorAllocator()
	{
		Assert(mActiveBlocks.empty());
	}

	void D3D12DescriptorAllocator::UpdateCompletedFenceValue(u64 val)
	{
		mBlockPool.UpdateTime(val);
	}

	void D3D12DescriptorAllocator::ReleaseCpuDesc(u64 fenceValue, const DescriptorPtr& item)
	{
		if (item.mBlock)
		{
			Assert(std::find(mActiveBlocks.begin(), mActiveBlocks.end(), item.mBlock) != mActiveBlocks.end());
			item.mBlock->ReleaseCpuDesc(item.mIndex, item.mIndex);
		}

		for (auto it = mActiveBlocks.begin(); it != mActiveBlocks.end(); )
		{
			DescriptorPool* block = *it;
			if (block->AllFreed())
			{
				mBlockPool.ScheduleReleaseItemAtTimestamp(fenceValue, block);
				it = mActiveBlocks.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	DescriptorPtr D3D12DescriptorAllocator::AllocCpuDesc()
	{
		for (DescriptorPool* block : mActiveBlocks)
		{
			if (block->CanAlloc())
			{
				const auto& index = block->AllocCpuDesc();
				return { block, index };
			}
		}

		DescriptorPool* newBlock = mBlockPool.AllocItem();
		mActiveBlocks.push_back(newBlock);

		return { newBlock, newBlock->AllocCpuDesc() };
	}
}