#include "D3D12BackendPch.h"
#include "D3D12DescriptorAllocator.h"
#include "D3D12Utils.h"

//////////////////////////////////////////////////////////////////////////

D3D12DescriptorBlock::D3D12DescriptorBlock(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const i32 numDescriptors)
	: mDesc{ type, UINT(numDescriptors), D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 }
	, mDescriptorHeap(nullptr)
	, mDescriptorNum(numDescriptors)
	, mDescriptorSize(device->GetDescriptorHandleIncrementSize(type))
	, mAllocator(numDescriptors)
{
	AssertHResultOk(device->CreateDescriptorHeap(&mDesc, IID_PPV_ARGS(&mDescriptorHeap)));
	mDescriptorHeap->SetName(L"D3D12DescriptorBlock");

	mCpuBase = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12DescriptorBlock::~D3D12DescriptorBlock()
{
	mDescriptorHeap->Release();
	mDescriptorHeap = nullptr;
	mCpuBase = {};
}


D3D12DescriptorBlock::DescHandleIndex D3D12DescriptorBlock::AllocCpuDesc()
{
	return mAllocator.Pop();
}

void D3D12DescriptorBlock::ReleaseCpuDesc(u64 fenceValue, const DescHandleIndex& index)
{
	mAllocator.Push(index);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorBlock::GetDesc(const DescHandleIndex& index) const
{
	if (0 <= index && index < mDescriptorNum)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE result(mCpuBase);
		result.Offset(index, mDescriptorSize);
		return result;
	}
	else
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());
	}
}

//////////////////////////////////////////////////////////////////////////

CD3DX12_CPU_DESCRIPTOR_HANDLE CpuDescItem::Get() const
{
	return mBlock ? mBlock->GetDesc(mIndex) : CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());
}

//////////////////////////////////////////////////////////////////////////

D3D12DescriptorAllocator::D3D12DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type)
	: mDescriptorType(type)
	, mDevice(device)
	, mBlockPool(
		[device, type]() { return new D3D12DescriptorBlock(device, type, msDescriptorBlockSize); },
		[](D3D12DescriptorBlock* t) {},
		[](D3D12DescriptorBlock* t) { if (t) { delete t; } }
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

void D3D12DescriptorAllocator::ReleaseCpuDesc(u64 fenceValue, const CpuDescItem& item)
{
	if (item.mBlock)
	{
		Assert(std::find(mActiveBlocks.begin(), mActiveBlocks.end(), item.mBlock) != mActiveBlocks.end());
		item.mBlock->ReleaseCpuDesc(item.mIndex, item.mIndex);
	}

	for (auto it = mActiveBlocks.begin(); it != mActiveBlocks.end(); )
	{
		D3D12DescriptorBlock* block = *it;
		if (block->AllFreed())
		{
			mBlockPool.ReleaseItem(fenceValue, block);
			it = mActiveBlocks.erase(it);
		}
		else
		{
			++it;
		}
	}
}

CpuDescItem D3D12DescriptorAllocator::AllocCpuDesc()
{
	for (D3D12DescriptorBlock* block : mActiveBlocks)
	{
		if (block->CanAlloc())
		{
			const auto& index = block->AllocCpuDesc();
			return { block, index };
		}
	}

	D3D12DescriptorBlock* newBlock = mBlockPool.AllocItem();
	mActiveBlocks.push_back(newBlock);

	return { newBlock, newBlock->AllocCpuDesc() };
}
