#include "RenderPch.h"
#include "D3D12RenderTarget.h"

SwapChainBufferResource::SwapChainBufferResource(D3D12Device* device, ID3D12Resource* res)
	: mResource(res)
{
	const auto& desc = mResource->GetDesc();
	m_width = desc.Width;
	m_height = desc.Height;

	NAME_RAW_D3D12_OBJECT(mResource, L"SwapChainBuffers");

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
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mResource, mResStates, destState));
		mResStates = destState;
	}
}

SwapChainBuffers::SwapChainBuffers(D3D12Device* device, IDXGISwapChain3* swapChain, const int32_t frameCount)
	: mDevice(device)
	, mFrameCount(frameCount)
	, mRenderTargets(frameCount)
{
	for (int32_t n = 0; n < mFrameCount; n++)
	{
		ID3D12Resource* rt = nullptr;
		AssertHResultOk(swapChain->GetBuffer(n, IID_PPV_ARGS(&rt)));

		mRenderTargets[n] = new SwapChainBufferResource(device, rt);
	}
}

SwapChainBufferResource* SwapChainBuffers::GetBuffer() const
{
	return mRenderTargets[mCurrentBackBufferIndex];
}
