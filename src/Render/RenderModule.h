#pragma once

#include "windows.h"

class D3D12Device;
class ScreenRenderer;
class RenderDocIntegration;

class GD_RENDER_API RenderModule
{
public:
	RenderModule(HWND window);

	void TickFrame();

	D3D12Device* GetDevice() const { return mDevice; }

protected:
	HWND		mWindow = {};
	D3D12Device* mDevice = nullptr;
	RenderDocIntegration* mRenderDoc = nullptr;
	std::unique_ptr<ScreenRenderer>	mScreenRenderer;
};

