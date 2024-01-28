#pragma once

#include "D3D12Headers.h"
#include "D3D12CommandContext.h"

namespace D3D12Backend
{
	class D3D12CommandContext;
	class GraphicsContext;
	class ComputeContext;
	class D3D12Device;

	class GD_D3D12BACKEND_API D3D12GpuQueue
	{
	public:
		D3D12GpuQueue(D3D12Device* device, D3D12GpuQueueType type, const char* name);
		virtual ~D3D12GpuQueue();

		GraphicsContext* AllocGraphicContext();
		ComputeContext* AllocComputeContext();

		u64						GetGpuPlannedValue() const { return mGpuPlannedValue; }
		u64						GetGpuCompletedValue() const { return mGpuCompletedValue; }
		D3D12GpuQueueType		GetType() const { return mType; }
		ID3D12CommandQueue* GetCommandQueue() const { return mCommandQueue; }

		void					Execute();

		void					CpuWaitForThisQueue(u64 value);
		void					IncreaseGpuPlannedValue(u64 value);
		bool					IsGpuValueFinished(u64 value);

		static D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(D3D12GpuQueueType type);

	protected:
		D3D12GpuQueueType const	mType = D3D12GpuQueueType::Graphic;
		D3D12Device* const		mDevice = nullptr;
		std::string				mName;

		ID3D12CommandQueue* mCommandQueue = nullptr;
		ID3D12Fence* mFence = nullptr;

		u64						mGpuPlannedValue = 1;
		u64						mGpuCompletedValue = 0;

		HANDLE					mCpuEventHandle = {};

		SuspendedReleasePool<GraphicsContext>* mGraphicContextPool = nullptr;
		SuspendedReleasePool<ComputeContext>* mComputeContextPool = nullptr;
	};

}