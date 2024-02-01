#pragma once

#include "D3D12Headers.h"
#include "Common/Math.h"
#include "D3D12Device.h"
#include "D3D12Resource.h"

namespace D3D12Backend
{
	class D3D12Device;

	class SwapChain
	{
	public:
		SwapChain(D3D12Device* device, HWND windowHandle, const Vec2u& size, const u32 frameCount);
		SwapChain(const SwapChain& other) = delete;

		GI::IGraphicMemoryResource*		GetBuffer() const;
		HWND					GetWindowHandle() const { return mWindowHandle; }
		void					Present();
		Vec2u					GetSize() const { return mSize; }

		void					Resize(const Vec2u& newSize);

		void					ClearBuffers();
		void					InitialBuffers();

	protected:
		D3D12Device* const						mDevice = nullptr;
		IDXGISwapChain1*						mSwapChain = nullptr;
		HWND									mWindowHandle = {};
		const DXGI_FORMAT						mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		const u32								mFrameCount;
		i32										mCurrentBackBufferIndex = 0;
		std::vector<std::unique_ptr<GI::IGraphicMemoryResource>>
												mBuffers;
		Vec2u									mSize = {};
	};
}
