#pragma once

#include "RenderMacros.h"
#include "windows.h"

class D3D12Device;

class GD_RENDER_API RenderModule
{
public:
	RenderModule(HWND window);

	void Present();

protected:
	HWND		mWindow = {};
	D3D12Device* mDevice = nullptr;
};

