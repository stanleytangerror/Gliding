#include "D3D12BackendPch.h"
#include "D3D12SwapChain.h"

namespace D3D12Backend
{
	SwapChainBufferResource::SwapChainBufferResource(D3D12Device* device, ID3D12Resource* res, const char* name)
		: mName(name)
	{
		mResource = D3D12Backend::CommitedResource::CommitedResource::Possessor()
			.SetName(name)
			.SetCurrentState(D3D12_RESOURCE_STATE_COMMON)
			.SetResource(res)
			.Possess(device);

		mRtv
			.SetResource(mResource)
			.SetFormat(mResource->GetFormat())
			.SetViewDimension(GI::RtvDimension::TEXTURE2D);
	}

	void SwapChainBufferResource::PrepareForPresent(D3D12Backend::D3D12CommandContext* context)
	{
		mResource->Transition(context, D3D12_RESOURCE_STATE_PRESENT);
	}

	Vec3i SwapChainBufferResource::GetSize() const
	{
		return mResource->GetSize();
	}

	SwapChainBuffers::SwapChainBuffers(D3D12Device* device, IDXGISwapChain3* swapChain, const int32_t frameCount)
		: mDevice(device)
		, mSwapChain(swapChain)
		, mFrameCount(frameCount)
		, mRenderTargets(frameCount)
	{
		for (int32_t n = 0; n < mFrameCount; n++)
		{
			ID3D12Resource* rt = nullptr;
			AssertHResultOk(swapChain->GetBuffer(n, IID_PPV_ARGS(&rt)));

			mRenderTargets[n] = new SwapChainBufferResource(device, rt, Utils::FormatString("BackBuffer_%d", n).c_str());
		}

		mSize = mRenderTargets[0]->GetSize();
	}

	SwapChainBufferResource* SwapChainBuffers::GetBuffer() const
	{
		return mRenderTargets[mCurrentBackBufferIndex];
	}

	void SwapChainBuffers::Present()
	{
		DEBUG_PRINT(" ================ Begin Present ===================== ");
		DEBUG_PRINT("\t Before present, current Back Buffer Index % d", mSwapChain->GetCurrentBackBufferIndex());

		AssertHResultOk(mSwapChain->Present(0, 0));
		mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % mFrameCount;

		DEBUG_PRINT("\t Done present, current Back Buffer Index % d", mSwapChain->GetCurrentBackBufferIndex());
		DEBUG_PRINT(" ================ End Present ======================= ");
	}
}