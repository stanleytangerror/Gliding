#include "RenderPch.h"
#include "D3D12CommandContext.h"
#include "D3D12DescriptorAllocator.h"
#include "D3D12DescriptorHeap.h"

D3D12CommandContext::D3D12CommandContext(D3D12Device* device, D3D12GpuQueue* gpuQueue)
	: mDevice(device)
	, mGpuQueue(gpuQueue)
	, mThisCpuThreadId(std::this_thread::get_id())
{
	const D3D12_COMMAND_LIST_TYPE type = D3D12GpuQueue::GetD3D12CommandListType(mGpuQueue->GetType());

	AssertHResultOk(mDevice->GetDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&mCommandAllocator)));
	AssertHResultOk(mDevice->GetDevice()->CreateCommandList(0, type, mCommandAllocator, nullptr, IID_PPV_ARGS(&mCommandList)));

	mCommandAllocator->SetName(Utils::ToWString(Utils::FormatString("CommandAllocator_%d", mGpuQueue->GetGpuPlannedValue())).c_str());
	mCommandList->SetName(Utils::ToWString(Utils::FormatString("CommandList_%d", mGpuQueue->GetGpuPlannedValue())).c_str());

	mConstantBuffer = new D3D12ConstantBuffer(mDevice);

	for (i32 i = 0; i < mRuntimeDescHeaps.size(); ++i)
	{
		mRuntimeDescHeaps[i] = new RuntimeDescriptorHeap(mDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE(i));
	}
}

void D3D12CommandContext::Finalize()
{
	AssertInThread(mThisCpuThreadId);

	AssertHResultOk(mCommandList->Close());
}

void D3D12CommandContext::Reset()
{
	AssertInThread(mThisCpuThreadId);
	
	AssertHResultOk(mCommandAllocator->Reset());
	AssertHResultOk(mCommandList->Reset(mCommandAllocator, nullptr));

	mCommandAllocator->SetName(Utils::ToWString(Utils::FormatString("CommandAllocator_%d", mGpuQueue->GetGpuPlannedValue())).c_str());
	mCommandList->SetName(Utils::ToWString(Utils::FormatString("CommandList_%d", mGpuQueue->GetGpuPlannedValue())).c_str());

	mConstantBuffer->Reset();
	for (const auto& heap : mRuntimeDescHeaps)
	{
		heap->Reset();
	}
}

u64 D3D12CommandContext::GetPlannedFenceValue() const
{
	AssertInThread(mThisCpuThreadId);
	
	return mGpuQueue->GetGpuPlannedValue();
}

void D3D12CommandContext::Transition(ID3D12Resource* resource, const D3D12_RESOURCE_STATES srcState, const D3D12_RESOURCE_STATES destState)
{
	AssertInThread(mThisCpuThreadId);
	
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

GraphicsContext::GraphicsContext(D3D12Device* device, D3D12GpuQueue* gpuQueue)
	: D3D12CommandContext(device, gpuQueue)
{

}

ComputeContext::ComputeContext(D3D12Device* device, D3D12GpuQueue* gpuQueue)
	: D3D12CommandContext(device, gpuQueue)
{

}
