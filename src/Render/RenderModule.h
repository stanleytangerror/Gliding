#pragma once

#include "D3D12/D3D12Device.h"

class GD_RENDER_API RenderModule
{
public:
	RenderModule(HWND window);

	void Present();

protected:
	HWND		mWindow = {};
	D3D12Device* mDevice = nullptr;
};

