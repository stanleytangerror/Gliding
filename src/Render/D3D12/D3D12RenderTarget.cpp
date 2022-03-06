#include "RenderPch.h"
#include "D3D12RenderTarget.h"

SwapChainBufferResource::SwapChainBufferResource(D3D12Device* device, ID3D12Resource* res, const char* name)
	: mResource(res)
	, mName(name)
{
	const auto& desc = mResource->GetDesc();
	mWidth = desc.Width;
	mHeight = desc.Height;

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

void SwapChainBufferResource::Transition(ID3D12GraphicsCommandList* commandList, const D3D12_RESOURCE_STATES& destState)
{
	if (mResStates != destState)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource, mResStates, destState);
		commandList->ResourceBarrier(1, &barrier);
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
	DEBUG_PRINT("Current Back Buffer Index %d", mSwapChain->GetCurrentBackBufferIndex());

	AssertHResultOk(mSwapChain->Present(1, 0));
	mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % mFrameCount;
}
