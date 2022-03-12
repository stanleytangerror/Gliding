#pragma once

#include "D3D12Headers.h"
#include "Common/Math.h"
#include "D3D12Device.h"
#include "D3D12ResourceView.h"
#include "D3D12Resource.h"

class D3D12Device;
class RTV;

class SwapChainBufferResource : public ID3D12Res
{
public:
	SwapChainBufferResource(D3D12Device* device, ID3D12Resource* res, const char* name);

	ID3D12Resource* GetD3D12Resource() const override { return mResource; }
	Vec3i				GetSize() const override { return { mWidth, mHeight, 0 }; }
	std::string			GetName() const override;

	RTV* GetRtv() const { return mRtv; }
	void Transition(ID3D12GraphicsCommandList* commandList, const D3D12_RESOURCE_STATES& destState) override;

protected:
	ID3D12Resource* const	mResource = nullptr;
	D3D12_RESOURCE_STATES	mResStates = D3D12_RESOURCE_STATE_COMMON;
	RTV* mRtv = nullptr;

public:
	std::string const		mName;
	int32_t					mWidth = 0;
	int32_t					mHeight = 0;
};

class SwapChainBuffers
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

