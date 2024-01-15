#include "D3D12BackendPch.h"
#include "D3D12CommandContext.h"
#include "D3D12DescriptorAllocator.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Resource.h"

namespace D3D12Backend
{
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

		mConstantBuffer = std::make_unique<D3D12ConstantBuffer>(mDevice);

		for (i32 i = 0; i < mRuntimeDescHeaps.size(); ++i)
		{
			mRuntimeDescHeaps[i] = std::make_unique<RuntimeDescriptorHeap>(mDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE(i));
		}
	}

	D3D12CommandContext::~D3D12CommandContext()
	{
		mCommandList->Release();
		mCommandAllocator->Release();
	}

	void D3D12CommandContext::Finalize()
	{
		PROFILE_EVENT(D3D12CommandContext::Finalize);

		AssertInThread(mThisCpuThreadId);

		AssertHResultOk(mCommandList->Close());
	}

	void D3D12CommandContext::Reset()
	{
		// TODO: add thread check with current thread + destroy thread (main thread)
		//AssertInThread(mThisCpuThreadId);

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

	void D3D12CommandContext::CopyResource(GI::IGraphicMemoryResource* dst, GI::IGraphicMemoryResource* src)
	{
		auto destRes = reinterpret_cast<CommitedResource*>(dst);
		auto srcRes = reinterpret_cast<CommitedResource*>(src);

		Assert(destRes->GetDimSize() == srcRes->GetDimSize());
		Assert(destRes->GetD3D12Resource()->GetDesc() == destRes->GetD3D12Resource()->GetDesc());

		destRes->Transition(this, D3D12_RESOURCE_STATE_COPY_DEST);
		srcRes->Transition(this, D3D12_RESOURCE_STATE_COPY_SOURCE);

		mCommandList->CopyResource(destRes->GetD3D12Resource(), srcRes->GetD3D12Resource());
	}

	void D3D12CommandContext::CopyBuffer2D(GI::IGraphicMemoryResource* dst, GI::IGraphicMemoryResource* src)
	{
		auto destRes = reinterpret_cast<CommitedResource*>(dst);
		auto srcRes = reinterpret_cast<CommitedResource*>(src);

		Assert(destRes->GetDimSize() == srcRes->GetDimSize());

		destRes->Transition(this, D3D12_RESOURCE_STATE_COPY_DEST);
		srcRes->Transition(this, D3D12_RESOURCE_STATE_COPY_SOURCE);

		auto desc = srcRes->GetD3D12Resource()->GetDesc();

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		{
			dstLocation.pResource = destRes->GetD3D12Resource();
			dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLocation.SubresourceIndex = 2;
		}

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		{
			srcLocation.pResource = srcRes->GetD3D12Resource();
			srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			srcLocation.SubresourceIndex = 2;
		}

		const auto& size = srcRes->GetDimSize();
		D3D12_BOX box = { 0, 0, 0, size.x(), size.y(), size.z() };
		mCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, &box);
	}

	GraphicsContext::GraphicsContext(D3D12Device* device, D3D12GpuQueue* gpuQueue)
		: D3D12CommandContext(device, gpuQueue)
	{

	}

	ComputeContext::ComputeContext(D3D12Device* device, D3D12GpuQueue* gpuQueue)
		: D3D12CommandContext(device, gpuQueue)
	{

	}
}