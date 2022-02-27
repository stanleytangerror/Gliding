#include "RenderPch.h"
#include "D3D12Fence.h"
#include "Common/AssertUtils.h"

D3D12Fence::D3D12Fence(ID3D12CommandQueue* q)
	: mQueue(q)
{
	ID3D12Device* device = nullptr;
	mQueue->GetDevice(__uuidof(*device), reinterpret_cast<void**>(&device));
	AssertHResultOk(device->CreateFence(mCpuFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

D3D12Fence::~D3D12Fence()
{
	mFence->Release();
}

void D3D12Fence::CpuWaitForGpuQueue()
{
	if (mGpuQueueFenceStatus.mCompletedValue < mGpuQueueFenceStatus.mPlannedValue)
	{
		HANDLE cpuWaitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		Assert(cpuWaitEvent);

		AssertHResultOk(mFence->SetEventOnCompletion(mGpuQueueFenceStatus.mPlannedValue, cpuWaitEvent));
		WaitForSingleObject(cpuWaitEvent, INFINITE);

		CloseHandle(cpuWaitEvent);

		mGpuQueueFenceStatus.mCompletedValue = mGpuQueueFenceStatus.mPlannedValue;
	}
}

void D3D12Fence::PlanGpuQueueWork()
{
	mGpuQueueFenceStatus.mPlannedValue = mCpuFenceValue;

	mQueue->Signal(mFence, mGpuQueueFenceStatus.mPlannedValue);
}

void D3D12Fence::IncreaseCpuFence()
{
	++mCpuFenceValue;
}

u64 D3D12Fence::GetPlannedValue() const
{
	return mGpuQueueFenceStatus.mPlannedValue;
}
