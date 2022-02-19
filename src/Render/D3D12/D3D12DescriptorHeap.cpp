#include "D3D12DescriptorHeap.h"
#include "D3D12/D3D12Utils.h"

//////////////////////////////////////////////////////////////////////////

DescriptorHeapBlock::DescriptorHeapBlock(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const bool shaderVisible, const i32 numDescriptors)
	: mDesc{ type, UINT(numDescriptors), shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 }
	, m_DescriptorHeap(nullptr)
	, mDescriptorNum(numDescriptors)
	, m_DescriptorSize(device->GetDescriptorHandleIncrementSize(type))
{
	AssertHResultOk(device->CreateDescriptorHeap(&mDesc, IID_PPV_ARGS(&m_DescriptorHeap)));
	NAME_RAW_D3D12_OBJECT(m_DescriptorHeap, L"RuntimeDescriptorHeap");

	m_CpuBase = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_GpuBase = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}


DescriptorHeapBlock::~DescriptorHeapBlock()
{
	m_DescriptorHeap->Release();
	m_DescriptorHeap = nullptr;
	m_CpuBase = {};
	m_GpuBase = {};
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeapBlock::GetCpuBaseWithOffset(i32 offset) const
{
	if (0 <= offset && offset < mDescriptorNum)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE result(m_CpuBase);
		result.Offset(m_DescriptorSize * offset);
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
		CD3DX12_GPU_DESCRIPTOR_HANDLE result(m_GpuBase);
		result.Offset(m_DescriptorSize * offset);
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

}

void RuntimeDescriptorHeap::Push(const int32_t handleCount, const D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescHandles, const u64 fenceValue)
{
	if (handleCount <= 0) { Assert(false); }

	if (!mCurrentWorkingBlock)
	{
		mCurrentWorkingBlock = mPool.AllocItem();
	}
	else if (mCurrentWorkingBlock || mCurrentWorkingIndex + handleCount > mCurrentWorkingBlock->GetNumDescriptos())
	{
		mPool.ReleaseItem(fenceValue, mCurrentWorkingBlock);
		mCurrentWorkingBlock = mPool.AllocItem();
	}

	for (i32 i = 0; i < handleCount; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = mCurrentWorkingBlock->GetCpuBaseWithOffset(mCurrentWorkingIndex + i);
		mDevice->CopyDescriptorsSimple(1, dstHandle, cpuDescHandles[i], mDescriptorType);
	}
}

void RuntimeDescriptorHeap::Retire(const int32_t handleCount)
{
	mCurrentWorkingIndex += handleCount;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE RuntimeDescriptorHeap::GetGpuHandle(const int32_t offset) const
{
	return mCurrentWorkingBlock ? mCurrentWorkingBlock->GetGpuBaseWithOffset(offset) : CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());
}

void RuntimeDescriptorHeap::UpdateCompletedFenceValue(u64 val)
{
	mPool.UpdateTime(val);
}
