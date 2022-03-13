#pragma once

#include "windows.h"
#include "WorldRenderer.h"
#include "ScreenRenderer.h"

class ScreenRenderer;
class RenderDocIntegration;
class WorldRenderer;
class D3D12RenderTarget;
class D3D12Device;

struct GD_RENDER_API WindowInfo
{
	HWND	mWindow = {};
	Vec2i	mSize = {};
};

class GD_RENDER_API RenderModule
{
public:
	RenderModule(WindowInfo windowInfo);

	void TickFrame(Timer* timer);

	D3D12Device* GetDevice() const { return mDevice; }

	WindowInfo			GetWindowInfo() const { return mWindowInfo; }
	Vec2i				GetBackBufferSize() const { return mBackBufferSize; }

protected:
	WindowInfo			mWindowInfo = {};
	Vec2i				mBackBufferSize = {};

	D3D12Device* mDevice = nullptr;
	RenderDocIntegration* mRenderDoc = nullptr;

	std::unique_ptr<ScreenRenderer>	mScreenRenderer;
	std::unique_ptr<WorldRenderer>	mWorldRenderer;

	D3D12RenderTarget*				mSceneHdrRt = nullptr;
};

