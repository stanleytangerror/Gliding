#include "RenderPch.h"
#include "RenderModule.h"
#include "D3D12/D3D12Device.h"
#include "ScreenRenderer.h"
#include "RenderDoc/RenderDocIntegration.h"

RenderModule::RenderModule(HWND window)
	: mWindow(window)
{
	mRenderDoc = new RenderDocIntegration();

	mDevice = new D3D12Device(window);

	mScreenRenderer = std::make_unique<ScreenRenderer>(this);
	mScreenRenderer->Initial();
}

void RenderModule::TickFrame(Timer* timer)
{
	mRenderDoc->OnStartFrame(mDevice, mWindow);

	mScreenRenderer->TickFrame(timer);

	GraphicsContext* context = mDevice->GetGraphicContextPool()->AllocItem();
	mScreenRenderer->Render(context);
	context->Execute();

	mDevice->Present();

	mRenderDoc->OnEndFrame(mDevice, mWindow);
}
