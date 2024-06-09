#pragma once

#include "D3D12Headers.h"
#include "D3D12CommandContext.h"
#include "D3D12SwapChain.h"

namespace D3D12Backend
{
	class D3D12CommandContext;
	class GraphicsContext;
	class ComputeContext;
	class D3D12Device;
	class SwapChain;

	class D3D12GpuQueue
	{
	public:
		D3D12GpuQueue(D3D12Device* device, D3D12GpuQueueType type, const char* name);
		virtual ~D3D12GpuQueue();

		GraphicsContext*		AllocGraphicContext();
		ComputeContext*			AllocComputeContext();

		u64						GetGpuPlannedValue() const { return mGpuPlannedValue; }
		u64						GetGpuCompletedValue() const { return mGpuCompletedValue; }
		D3D12GpuQueueType		GetType() const { return mType; }
		ID3D12CommandQueue*		GetCommandQueue() const { return mCommandQueue; }

		void					Execute();

		void					CpuWaitForThisQueue(u64 value);
		void					IncreaseGpuPlannedValue(u64 value);
		bool					IsGpuValueFinished(u64 value);

		SwapChain*				CreateSwapChain(u32 windowId, HWND windowHandle, const Vec2u& size, const int32_t frameCount);
		void					ReleaseSwapChainResources();
		std::vector<SwapChain*>	GetSwapChains() const;
		SwapChain*				GetSwapChain(u32 windowId) const;

		static D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(D3D12GpuQueueType type);

	protected:
		ID3D12Fence*			AllocFence();

	protected:
		D3D12GpuQueueType const	mType = D3D12GpuQueueType::Graphic;
		D3D12Device* const		mDevice = nullptr;
		std::string				mName;

		ID3D12CommandQueue* mCommandQueue = nullptr;
		std::vector<ID3D12Fence*> mAvailableFences;
		std::map<u64, ID3D12Fence*> mWorkingFences;

		u64						mGpuPlannedValue = 1;
		u64						mGpuCompletedValue = 0;

		SuspendedReleasePool<GraphicsContext>* mGraphicContextPool = nullptr;
		SuspendedReleasePool<ComputeContext>* mComputeContextPool = nullptr;

		std::map<u32, std::unique_ptr<SwapChain>>	mSwapChains;
	};

}