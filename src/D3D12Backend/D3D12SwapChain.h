#pragma once

#include "D3D12Headers.h"
#include "Common/Math.h"
#include "D3D12Device.h"
#include "D3D12Resource.h"

namespace D3D12Backend
{
	class D3D12Device;

	class GD_D3D12BACKEND_API SwapChain
	{
	public:
		SwapChain(D3D12Device* device, HWND windowHandle, const Vec2i& size, const int32_t frameCount);

		CommitedResource*		GetBuffer() const;

		void					Present();

	protected:
		D3D12Device* const						mDevice = nullptr;
		IDXGISwapChain3*						mSwapChain = nullptr;
		const int32_t							mFrameCount;
		i32										mCurrentBackBufferIndex = 0;
		std::vector<CommitedResource*>			mBuffers;
		Vec2i									mSize = {};
	};
}
