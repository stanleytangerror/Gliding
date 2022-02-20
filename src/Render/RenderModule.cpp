#include "RenderPch.h"
#include "RenderModule.h"

RenderModule::RenderModule(HWND window)
	: mWindow(window)
	, mDevice(new D3D12Device(window))
{

}

void RenderModule::Present()
{
	mDevice->Present();
}
