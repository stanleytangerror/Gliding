#include "RenderPch.h"
#include "RenderModule.h"
#include "D3D12/D3D12Device.h"
#include "ScreenRenderer.h"

RenderModule::RenderModule(HWND window)
	: mWindow(window)
	, mDevice(new D3D12Device(window))
{
	mScreenRenderer = std::make_unique<ScreenRenderer>(this);
	mScreenRenderer->Initial();
}

void RenderModule::TickFrame()
{
	GraphicsContext* context = mDevice->GetGraphicContextPool()->AllocItem();
	
	mScreenRenderer->Render(context);

	context->Execute();

	mDevice->Present();
}
