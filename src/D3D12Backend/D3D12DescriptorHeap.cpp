#include "D3D12BackendPch.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Utils.h"

namespace D3D12Backend
{
	//////////////////////////////////////////////////////////////////////////

	DescriptorArrayResource::DescriptorArrayResource(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const bool deviceVisible, const i32 capacity, const char* name)
		: mDesc{ type, UINT(capacity), deviceVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 }
		, mDescriptorHeap(nullptr)
		, mCapacity(capacity)
		, mStride(device->GetDescriptorHandleIncrementSize(type))
	{
		AssertHResultOk(device->CreateDescriptorHeap(&mDesc, IID_PPV_ARGS(&mDescriptorHeap)));
		mDescriptorHeap->SetName(Utils::ToWString(name).c_str());

		mCpuBase = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		mGpuBase = mDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	}


	DescriptorArrayResource::~DescriptorArrayResource()
	{
		mDescriptorHeap->Release();
		mDescriptorHeap = nullptr;
		mCpuBase = {};
		mGpuBase = {};
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorArrayResource::GetCpuBaseWithOffset(i32 index) const
	{
		if (0 <= index && index < mCapacity)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE result(mCpuBase);
			result.Offset(index, mStride);
			return result;
		}
		else
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());
		}
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorArrayResource::GetGpuBaseWithOffset(i32 index) const
	{
		if (0 <= index && index < mCapacity)
		{
			CD3DX12_GPU_DESCRIPTOR_HANDLE result(mGpuBase);
			result.Offset(index, mStride);
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
			[device, type]() { return new DescriptorArrayResource(device, type, true, msNumDescriptorsPerBlock, "Runtime descriptor heap"); },
			[](DescriptorArrayResource* t) {},
			[](DescriptorArrayResource* t) { if (t) { delete t; } }
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

		if (!mCurrentWorkingBlock || mCurrentWorkingIndex + handleCount >= mCurrentWorkingBlock->GetCapacity())
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

	u32 RuntimeDescriptorHeap::GetDescHandleSize() const
	{
		return mDevice->GetDescriptorHandleIncrementSize(mDescriptorType);
	}

	void RuntimeDescriptorHeap::Reset()
	{
		mPool.ReleaseAllActiveItems();
		mCurrentWorkingBlock = nullptr;
		mCurrentWorkingIndex = 0;
	}
}