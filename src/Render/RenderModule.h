#pragma once

#include "RenderMacros.h"
#include "windows.h"

class D3D12Device;
class ScreenRenderer;

class GD_RENDER_API RenderModule
{
public:
	RenderModule(HWND window);

	void TickFrame();

protected:
	HWND		mWindow = {};
	D3D12Device* mDevice = nullptr;

	std::unique_ptr<ScreenRenderer>	mScreenRenderer;
};

