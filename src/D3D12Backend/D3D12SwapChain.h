#pragma once

#include "D3D12Headers.h"
#include "Common/Math.h"
#include "D3D12Device.h"
#include "D3D12ResourceView.h"
#include "D3D12Resource.h"

class D3D12Device;
class RTV;

class GD_D3D12BACKEND_API SwapChainBufferResource : public ID3D12Res
{
public:
	SwapChainBufferResource(D3D12Device* device, ID3D12Resource* res, const char* name);

	ID3D12Resource* GetD3D12Resource() const override { return mResource; }
	Vec3i				GetSize() const override { return { mWidth, mHeight, 1 }; }
	i32					GetWidth() const { return mWidth; }
	i32					GetHeight() const { return mHeight; }
	std::string			GetName() const override;

	RTV* GetRtv() const { return mRtv; }
	void Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) override;

protected:
	ID3D12Resource* const	mResource = nullptr;
	D3D12_RESOURCE_STATES	mResStates = D3D12_RESOURCE_STATE_COMMON;
	RTV* mRtv = nullptr;

public:
	std::string const		mName;
	i32						mWidth = 0;
	i32						mHeight = 0;
};

class GD_D3D12BACKEND_API SwapChainBuffers
{
public:
	SwapChainBuffers(D3D12Device* device, IDXGISwapChain3* swapChain, const int32_t frameCount);
	SwapChainBufferResource* GetBuffer() const;

	void Present();

protected:
	D3D12Device* const						mDevice = nullptr;
	IDXGISwapChain3* mSwapChain = nullptr;
	const int32_t							mFrameCount;
	i32										mCurrentBackBufferIndex = 0;
	std::vector<SwapChainBufferResource*>	mRenderTargets;
};

