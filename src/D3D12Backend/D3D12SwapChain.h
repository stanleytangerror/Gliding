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
		SwapChain(D3D12Device* device, HWND windowHandle, const Vec2i& size, const u32 frameCount);

		CommitedResource*		GetBuffer() const;
		HWND					GetWindowHandle() const { return mWindowHandle; }
		void					Present();
		Vec2i					GetSize() const { return mSize; }

		void					Resize(const Vec2i& newSize);

		void					ClearBuffers();
		void					InitialBuffers();

	protected:
		D3D12Device* const						mDevice = nullptr;
		IDXGISwapChain1*						mSwapChain = nullptr;
		HWND									mWindowHandle = {};
		const DXGI_FORMAT						mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		const u32								mFrameCount;
		i32										mCurrentBackBufferIndex = 0;
		std::vector<CommitedResource*>			mBuffers;
		Vec2i									mSize = {};
	};
}
