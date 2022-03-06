#include "RenderPch.h"
#include "D3D12GpuQueue.h"

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

	AssertHResultOk(mDevice->GetDevice()->CreateFence(mGpuPlannedValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	mGraphicContextPool = new SuspendedReleasePool<GraphicsContext>(
		[&]() { return new GraphicsContext(mDevice, this); },
		[](GraphicsContext* ctx) {},
		[](GraphicsContext* ctx) {});

	mComputeContextPool = new SuspendedReleasePool<ComputeContext>(
		[&]() { return new ComputeContext(mDevice, this); },
		[](ComputeContext* ctx) {},
		[](ComputeContext* ctx) {});
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

	mGraphicContextPool->ReleaseAllActiveItems(mGpuPlannedValue);
	mComputeContextPool->ReleaseAllActiveItems(mGpuPlannedValue);

	mGpuPlannedValue += 1;
}

D3D12_COMMAND_LIST_TYPE D3D12GpuQueue::GetD3D12CommandListType(D3D12GpuQueueType type)
{
	return 	
		type == D3D12GpuQueueType::Graphic ? D3D12_COMMAND_LIST_TYPE_DIRECT :
		type == D3D12GpuQueueType::Compute ? D3D12_COMMAND_LIST_TYPE_COMPUTE :
		type == D3D12GpuQueueType::Copy ? D3D12_COMMAND_LIST_TYPE_COPY : D3D12_COMMAND_LIST_TYPE_DIRECT;
}
