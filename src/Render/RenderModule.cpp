#include "RenderPch.h"
#include "RenderModule.h"
#include "D3D12/D3D12Device.h"

RenderModule::RenderModule(HWND window)
	: mWindow(window)
	, mDevice(new D3D12Device(window))
{

}

void RenderModule::Present()
{
	mDevice->Present();
}
