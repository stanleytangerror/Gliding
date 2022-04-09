#include "RenderPch.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12/D3D12Utils.h"

//////////////////////////////////////////////////////////////////////////

DescriptorHeapBlock::DescriptorHeapBlock(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const bool shaderVisible, const i32 numDescriptors)
	: mDesc{ type, UINT(numDescriptors), shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 }
	, mDescriptorHeap(nullptr)
	, mDescriptorNum(numDescriptors)
	, mDescriptorSize(device->GetDescriptorHandleIncrementSize(type))
{
	AssertHResultOk(device->CreateDescriptorHeap(&mDesc, IID_PPV_ARGS(&mDescriptorHeap)));
	mDescriptorHeap->SetName(L"RuntimeDescriptorHeap");

	mCpuBase = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	mGpuBase = mDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}


DescriptorHeapBlock::~DescriptorHeapBlock()
{
	mDescriptorHeap->Release();
	mDescriptorHeap = nullptr;
	mCpuBase = {};
	mGpuBase = {};
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeapBlock::GetCpuBaseWithOffset(i32 offset) const
{
	if (0 <= offset && offset < mDescriptorNum)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE result(mCpuBase);
		result.Offset(offset, mDescriptorSize);
		return result;
	}
	else
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());
	}
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeapBlock::GetGpuBaseWithOffset(i32 offset) const
{
	if (0 <= offset && offset < mDescriptorNum)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE result(mGpuBase);
		result.Offset(offset, mDescriptorSize);
		return result;
	}
	else
	{
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());
	}
}

//////////////////////////////////////////////////////////////////////////

RuntimeDescriptorHeap::RuntimeDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type)
	: mDescriptorType(type)
	, mDevice(device)
	, mPool(
		[device, type]() { return new DescriptorHeapBlock(device, type, true, msNumDescriptorsPerBlock); },
		[](DescriptorHeapBlock* t) {},
		[](DescriptorHeapBlock* t) { if (t) { delete t; } }
	)
{
}

RuntimeDescriptorHeap::~RuntimeDescriptorHeap()
{
	Reset();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE RuntimeDescriptorHeap::Push(const i32 handleCount, const D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescHandles)
{
	Assert(0 <= handleCount && handleCount < msNumDescriptorsPerBlock);

	if (!mCurrentWorkingBlock || mCurrentWorkingIndex + handleCount >= mCurrentWorkingBlock->GetNumDescriptos())
	{
		mCurrentWorkingBlock = mPool.AllocItem();
		mCurrentWorkingIndex = 0;
	}

	const CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescBaseAddr = mCurrentWorkingBlock->GetGpuBaseWithOffset(mCurrentWorkingIndex);

	for (i32 i = 0; i < handleCount; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = mCurrentWorkingBlock->GetCpuBaseWithOffset(mCurrentWorkingIndex + i);
		mDevice->CopyDescriptorsSimple(1, dstHandle, cpuDescHandles[i], mDescriptorType);
	}

	mCurrentWorkingIndex += handleCount;
	return gpuDescBaseAddr;
}

void RuntimeDescriptorHeap::Reset()
{
	mPool.ReleaseAllActiveItems();
	mCurrentWorkingBlock = nullptr;
	mCurrentWorkingIndex = 0;
}
