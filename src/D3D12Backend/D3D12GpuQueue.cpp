#include "D3D12BackendPch.h"
#include "D3D12GpuQueue.h"

//#define DEBUG_PRINT_COMMAND_QUEUE DEBUG_PRINT
#define DEBUG_PRINT_COMMAND_QUEUE(msg, ...) {}

namespace D3D12Backend
{
	void WaitForFenceCompletion(ID3D12Fence* fence, u64 value)
	{
		auto cpuEventHandle = CreateEvent(nullptr, false, false, nullptr);
		Assert(cpuEventHandle != INVALID_HANDLE_VALUE);

		AssertHResultOk(fence->SetEventOnCompletion(value, cpuEventHandle));
		WaitForSingleObject(cpuEventHandle, INFINITE);
		DEBUG_PRINT_COMMAND_QUEUE("Fence 0x%08x SetEventOnCompletion value %d", fence, value);

		CloseHandle(cpuEventHandle);

		DEBUG_PRINT_COMMAND_QUEUE("Fence 0x%08x GetCompletedValue %d", fence, fence->GetCompletedValue());
	}

	D3D12GpuQueue::D3D12GpuQueue(D3D12Device* device, D3D12GpuQueueType type, const char* name)
		: mDevice(device)
		, mName(name)
		, mType(type)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		{
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = GetD3D12CommandListType(mType);
		}
		AssertHResultOk(mDevice->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
		NAME_RAW_D3D12_OBJECT(mCommandQueue, mName.c_str());

		mGraphicContextPool = new SuspendedReleasePool<GraphicsContext>(
			[&]() { return new GraphicsContext(mDevice, this); },
			[](GraphicsContext* ctx) { ctx->Reset(); },
			[](GraphicsContext* ctx) {});

		mComputeContextPool = new SuspendedReleasePool<ComputeContext>(
			[&]() { return new ComputeContext(mDevice, this); },
			[](ComputeContext* ctx) {},
			[](ComputeContext* ctx) {});

		mCpuEventHandle = CreateEvent(nullptr, false, false, nullptr);
		Assert(mCpuEventHandle != INVALID_HANDLE_VALUE);
	}

	D3D12GpuQueue::~D3D12GpuQueue()
	{
		mGraphicContextPool->UpdateTime(mGpuCompletedValue);
		mComputeContextPool->UpdateTime(mGpuCompletedValue);

		Utils::SafeDelete(mGraphicContextPool);
		Utils::SafeDelete(mComputeContextPool);

		CloseHandle(mCpuEventHandle);

		//Utils::SafeRelease(mFence);
		Utils::SafeRelease(mCommandQueue);
	}

	GraphicsContext* D3D12GpuQueue::AllocGraphicContext()
	{
		return mGraphicContextPool->AllocItem();
	}

	ComputeContext* D3D12GpuQueue::AllocComputeContext()
	{
		return mComputeContextPool->AllocItem();
	}

	void D3D12GpuQueue::Execute()
	{
		std::vector<ID3D12CommandList*> cmdLists;

		for (GraphicsContext* ctx : mGraphicContextPool->GetAliveItems())
		{
			cmdLists.push_back(ctx->GetCommandList());
		}
		for (ComputeContext* ctx : mComputeContextPool->GetAliveItems())
		{
			cmdLists.push_back(ctx->GetCommandList());
		}

		mCommandQueue->ExecuteCommandLists(u32(cmdLists.size()), cmdLists.data());
		DEBUG_PRINT_COMMAND_QUEUE("[%s] ExecuteCommandLists length %d", mName.c_str(), cmdLists.size());

		auto fence = AllocFence();
		AssertHResultOk(mCommandQueue->Signal(fence, mGpuPlannedValue));
		DEBUG_PRINT_COMMAND_QUEUE("[%s] Signal fence 0x%08x value %d", mName.c_str(), fence, mGpuPlannedValue);

		Assert(mWorkingFences.find(mGpuPlannedValue) == mWorkingFences.end());
		mWorkingFences[mGpuPlannedValue] = fence;

		mGraphicContextPool->ScheduleReleaseAllActiveItemsAtTimestamp(mGpuPlannedValue);
		mComputeContextPool->ScheduleReleaseAllActiveItemsAtTimestamp(mGpuPlannedValue);
	}

	void D3D12GpuQueue::CpuWaitForThisQueue(u64 value)
	{
		Assert(value <= mGpuPlannedValue);

		//if (false) TODO use this line still works well
		if (value == GetGpuPlannedValue())
		{
			for (auto it = mWorkingFences.begin(); it != mWorkingFences.end(); ++it)
			{
				auto plannedValue = it->first;
				auto fence = it->second;
				const auto completedValue = fence->GetCompletedValue();

				WaitForFenceCompletion(fence, plannedValue);

				mAvailableFences.push_back(fence);
			}
			mWorkingFences.clear();

			{
				auto fence = AllocFence();
				AssertHResultOk(mCommandQueue->Signal(fence, value));
				DEBUG_PRINT_COMMAND_QUEUE("[%s] Signal fence 0x%08x value %d", mName.c_str(), fence, value);

				WaitForFenceCompletion(fence, value);
				mAvailableFences.push_back(fence);
			}
		}
		else
		{
			for (auto it = mWorkingFences.begin(); it != mWorkingFences.end(); )
			{
				auto plannedValue = it->first;
				auto fence = it->second;
				if (plannedValue <= value)
				{
					const auto completedValue = fence->GetCompletedValue();
					if (completedValue < plannedValue)
					{
						WaitForFenceCompletion(fence, plannedValue);
					}

					it = mWorkingFences.erase(it);
					mAvailableFences.push_back(fence);
				}
				else
				{
					++it;
				}
			}
		}


		mGpuCompletedValue = std::max(mGpuCompletedValue, value);

		mGraphicContextPool->UpdateTime(mGpuCompletedValue);
	}

	void D3D12GpuQueue::IncreaseGpuPlannedValue(u64 value)
	{
		mGpuPlannedValue += value;
	}

	bool D3D12GpuQueue::IsGpuValueFinished(u64 value)
	{
		return value <= mGpuCompletedValue;
	}

	D3D12_COMMAND_LIST_TYPE D3D12GpuQueue::GetD3D12CommandListType(D3D12GpuQueueType type)
	{
		return
			type == D3D12GpuQueueType::Graphic ? D3D12_COMMAND_LIST_TYPE_DIRECT :
			type == D3D12GpuQueueType::Compute ? D3D12_COMMAND_LIST_TYPE_COMPUTE :
			type == D3D12GpuQueueType::Copy ? D3D12_COMMAND_LIST_TYPE_COPY : D3D12_COMMAND_LIST_TYPE_DIRECT;
	}

	ID3D12Fence* D3D12GpuQueue::AllocFence()
	{
		if (mAvailableFences.empty())
		{
			ID3D12Fence* newFence = nullptr;
			AssertHResultOk(mDevice->GetDevice()->CreateFence(mGpuCompletedValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&newFence)));
			return newFence;
		}
		else
		{
			auto newFence = mAvailableFences.front();
			mAvailableFences.erase(mAvailableFences.begin());
			return newFence;
		}
	}
}