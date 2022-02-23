#pragma once

#include "D3D12/D3D12Headers.h"
#include "Common/CommonTypes.h"
#include "D3D12/D3D12Device.h"
#include <map>

class D3D12Device;

class D3D12Fence
{
public:
	D3D12Fence(ID3D12CommandQueue* q);
	virtual ~D3D12Fence();

	void CpuWaitForGpuQueue();
	void PlanGpuQueueWork();
	void IncreaseCpuFence();
private:
	ID3D12CommandQueue* const mQueue = nullptr;
	ID3D12Fence*		mFence = nullptr;
	u64					mCpuFenceValue = 0;

	struct CommandQueueFenceStatus
	{
		u64			mCompletedValue = 0;
		u64			mPlannedValue = 0;
	}					mGpuQueueFenceStatus;
};

