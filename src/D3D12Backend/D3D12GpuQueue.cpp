#include "D3D12BackendPch.h"
#include "D3D12GpuQueue.h"

namespace D3D12Backend
{
	D3D12GpuQueue::D3D12GpuQueue(D3D12Device* device, D3D12GpuQueueType type)
		: mDevice(device)
		, mType(type)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		{
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = GetD3D12CommandListType(mType);
		}
		AssertHResultOk(mDevice->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

		AssertHResultOk(mDevice->GetDevice()->CreateFence(mGpuCompletedValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

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

		Utils::SafeRelease(mFence);
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
		mCommandQueue->Signal(mFence, mGpuPlannedValue);

		mGraphicContextPool->ReleaseAllActiveItems(mGpuPlannedValue);
		mComputeContextPool->ReleaseAllActiveItems(mGpuPlannedValue);
	}

	void D3D12GpuQueue::CpuWaitForThisQueue(u64 value)
	{
		Assert(mGpuCompletedValue <= value && value <= mGpuPlannedValue);

		if (mFence->GetCompletedValue() < value)
		{
			DEBUG_PRINT("CPU wait for GPU completed %d, wait for %d", mFence->GetCompletedValue(), value);
			mFence->SetEventOnCompletion(value, mCpuEventHandle);
			WaitForSingleObject(mCpuEventHandle, INFINITE);
		}
		Assert(value <= mFence->GetCompletedValue());

		mGpuCompletedValue = mFence->GetCompletedValue();
		DEBUG_PRINT("GpuQueue complete value %d, planned value %d", mGpuCompletedValue, mGpuPlannedValue);

		mGraphicContextPool->UpdateTime(mGpuCompletedValue);
	}

	void D3D12GpuQueue::IncreaseGpuPlannedValue(u64 value)
	{
		mGpuPlannedValue += value;
	}

	bool D3D12GpuQueue::IsGpuValueFinished(u64 value)
	{
		return mGpuCompletedValue >= value;
	}

	D3D12_COMMAND_LIST_TYPE D3D12GpuQueue::GetD3D12CommandListType(D3D12GpuQueueType type)
	{
		return
			type == D3D12GpuQueueType::Graphic ? D3D12_COMMAND_LIST_TYPE_DIRECT :
			type == D3D12GpuQueueType::Compute ? D3D12_COMMAND_LIST_TYPE_COMPUTE :
			type == D3D12GpuQueueType::Copy ? D3D12_COMMAND_LIST_TYPE_COPY : D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
}