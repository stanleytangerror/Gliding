#include "RenderPch.h"
#include "D3D12CommandContext.h"
#include "D3D12DescriptorAllocator.h"
#include "D3D12DescriptorHeap.h"

GraphicsContext::GraphicsContext(D3D12Device* device, ID3D12CommandQueue* q, D3D12Fence* fence)
	: mDevice(device)
	, mQueue(q)
	, mFence(fence)
{
	mCurrentCommandAllocator = mDevice->GetCommandAllocatorPool()->AllocItem();
	AssertHResultOk(mDevice->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCurrentCommandAllocator, nullptr, IID_PPV_ARGS(&mCommandList)));

	mCommandList->Close();

	mRuntimeHeap = mDevice->GetRuntimeDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mConstantBuffer = new D3D12ConstantBuffer(mDevice);

	Reset();
}

void GraphicsContext::Execute()
{
	AssertHResultOk(mCommandList->Close());
	
	ID3D12CommandList* ppCommandLists[] = { mCommandList };
	mQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	mDevice->GetCommandAllocatorPool()->ReleaseItem(mFence->GetPlannedValue(), mCurrentCommandAllocator);
}

void GraphicsContext::Reset()
{
	if (!mCurrentCommandAllocator)
	{
		mCurrentCommandAllocator = mDevice->GetCommandAllocatorPool()->AllocItem();
	}

	AssertHResultOk(mCommandList->Reset(mCurrentCommandAllocator, nullptr));
}

u64 GraphicsContext::GetPlannedFenceValue() const
{
	return mDevice->GetFence()->GetPlannedValue();
}

void GraphicsContext::Transition(ID3D12Resource* resource, const D3D12_RESOURCE_STATES srcState, const D3D12_RESOURCE_STATES destState)
{
	if (srcState != destState)
	{
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		{
			BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			BarrierDesc.Transition.pResource = resource;
			BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			BarrierDesc.Transition.StateBefore = srcState;
			BarrierDesc.Transition.StateAfter = destState;
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		}

		mCommandList->ResourceBarrier(1, &BarrierDesc);
	}
}

/////////////////////////////////////////////////////////////////////


ComputeContext::ComputeContext(D3D12Device* device, ID3D12CommandQueue* q, D3D12Fence* fence)
	: mDevice(device)
	, mQueue(q)
	, mFence(fence)
{
	mCurrentCommandAllocator = mDevice->GetCommandAllocatorPool()->AllocItem();
	AssertHResultOk(mDevice->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCurrentCommandAllocator, nullptr, IID_PPV_ARGS(&mCommandList)));

	mCommandList->Close();

	mRuntimeHeap = mDevice->GetRuntimeDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mConstantBuffer = new D3D12ConstantBuffer(mDevice);
}

void ComputeContext::Execute()
{
	AssertHResultOk(mCommandList->Close());

	ID3D12CommandList* ppCommandLists[] = { mCommandList };
	mQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	mDevice->GetCommandAllocatorPool()->ReleaseItem(mFence->GetPlannedValue(), mCurrentCommandAllocator);
}

void ComputeContext::Reset()
{
	if (!mCurrentCommandAllocator)
	{
		mCurrentCommandAllocator = mDevice->GetCommandAllocatorPool()->AllocItem();
	}

	AssertHResultOk(mCommandList->Reset(mCurrentCommandAllocator, nullptr));
}

u64 ComputeContext::GetPlannedFenceValue() const
{
	return mDevice->GetFence()->GetPlannedValue();
}

//void ComputeContext::Transition(ID3D12Resource* resource, const D3D12_RESOURCE_STATES srcState, const D3D12_RESOURCE_STATES destState)
//{
//	if (srcState != destState)
//	{
//		D3D12_RESOURCE_BARRIER BarrierDesc = {};
//		{
//			BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//			BarrierDesc.Transition.pResource = resource;
//			BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
//			BarrierDesc.Transition.StateBefore = srcState;
//			BarrierDesc.Transition.StateAfter = destState;
//			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//		}
//
//		mCommandList->ResourceBarrier(1, &BarrierDesc);
//	}
//}

