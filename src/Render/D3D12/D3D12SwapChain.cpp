#include "RenderPch.h"
#include "D3D12SwapChain.h"

SwapChainBufferResource::SwapChainBufferResource(D3D12Device* device, ID3D12Resource* res, const char* name)
	: mResource(res)
	, mName(name)
{
	const auto& desc = mResource->GetDesc();
	mWidth = static_cast<i32>(desc.Width);
	mHeight = static_cast<i32>(desc.Height);

	NAME_RAW_D3D12_OBJECT(mResource, name);

	mResStates = D3D12_RESOURCE_STATE_COMMON;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	{
		rtvDesc.Format = mResource->GetDesc().Format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
	}

	mRtv = new RTV(device, this, rtvDesc);
}

void SwapChainBufferResource::Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState)
{
	if (mResStates != destState)
	{
		context->Transition(mResource, mResStates, destState);
		mResStates = destState;
	}
}

std::string SwapChainBufferResource::GetName() const
{
	return mName;
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
