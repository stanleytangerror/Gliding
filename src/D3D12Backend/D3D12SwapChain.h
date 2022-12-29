#pragma once

#include "D3D12Headers.h"
#include "Common/Math.h"
#include "D3D12Device.h"
#include "D3D12ResourceView.h"
#include "D3D12Resource.h"

class D3D12Device;
class D3D12CommandContext;
namespace D3D12Backend
{
	class RenderTargetView;
}

class GD_D3D12BACKEND_API SwapChainBufferResource
{
public:
	SwapChainBufferResource(D3D12Device* device, ID3D12Resource* res, const char* name);

	void					PrepareForPresent(D3D12CommandContext* context);

	D3D12Backend::RenderTargetView*					GetRtv() const { return mRtv; }
	Vec3i					GetSize() const;

protected:
	D3D12Backend::CommitedResource*	mResource = nullptr;
	D3D12Backend::RenderTargetView*					mRtv = nullptr;

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
	Vec3i					GetSize() const { return mSize; }

	void					Present();

protected:
	D3D12Device* const						mDevice = nullptr;
	IDXGISwapChain3* mSwapChain = nullptr;
	const int32_t							mFrameCount;
	i32										mCurrentBackBufferIndex = 0;
	std::vector<SwapChainBufferResource*>	mRenderTargets;
	Vec3i									mSize = {};
};

