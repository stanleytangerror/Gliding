#include "D3D12BackendPch.h"
#include "D3D12SwapChain.h"

namespace D3D12Backend
{
	SwapChain::SwapChain(D3D12Device* device, HWND windowHandle, const Vec2i& size, const int32_t frameCount)
		: mDevice(device)
		, mSize(size)
		, mFrameCount(frameCount)
	{
		// Describe and create the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = frameCount;
		swapChainDesc.Width = size.x();
		swapChainDesc.Height = size.y();
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		IDXGISwapChain1* swapChain1 = nullptr;
		AssertHResultOk(device->GetFactory()->CreateSwapChainForHwnd(
			device->GetGpuQueue(D3D12GpuQueueType::Graphic)->GetCommandQueue(),
			windowHandle,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain1
		));
		mSwapChain = reinterpret_cast<IDXGISwapChain3*>(swapChain1);

		for (auto n = 0; n < mFrameCount; n++)
		{
			ID3D12Resource* resource = nullptr;
			AssertHResultOk(mSwapChain->GetBuffer(n, IID_PPV_ARGS(&resource)));
			
			mBuffers.push_back(
				CommitedResource::Possessor()
				.SetName(Utils::FormatString("BackBuffer_%d", n).c_str())
				.SetCurrentState(D3D12_RESOURCE_STATE_COMMON)
				.SetResource(resource)
				.Possess(device));
		}
	}

	CommitedResource* SwapChain::GetBuffer() const
	{
		return mBuffers[mCurrentBackBufferIndex];
	}

	void SwapChain::Present()
	{
		DEBUG_PRINT(" ================ Begin Present ===================== ");
		DEBUG_PRINT("\t Before present, current Back Buffer Index % d", mSwapChain->GetCurrentBackBufferIndex());

		AssertHResultOk(mSwapChain->Present(0, 0));
		mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % mFrameCount;

		DEBUG_PRINT("\t Done present, current Back Buffer Index % d", mSwapChain->GetCurrentBackBufferIndex());
		DEBUG_PRINT(" ================ End Present ======================= ");
	}
}