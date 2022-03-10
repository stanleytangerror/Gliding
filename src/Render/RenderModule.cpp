#include "RenderPch.h"
#include "RenderModule.h"
#include "D3D12/D3D12Device.h"
#include "ScreenRenderer.h"
#include "RenderDoc/RenderDocIntegration.h"
#include "D3D12/D3D12RenderTarget.h"

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
	if (mRenderDoc)
	{
		mRenderDoc->OnStartFrame(mDevice, mWindow);
	}

	mScreenRenderer->TickFrame(timer);

	GraphicsContext* context = mDevice->GetGpuQueue(D3D12GpuQueueType::Graphic)->AllocGraphicContext();
	{
		SwapChainBufferResource* rtRes = mDevice->GetBackBuffer()->GetBuffer();
		mScreenRenderer->Render(context, rtRes->GetRtv());
		rtRes->Transition(context->GetCommandList(), D3D12_RESOURCE_STATE_PRESENT);
	}
	context->Finalize();

	mDevice->Present();

	if (mRenderDoc)
	{
		mRenderDoc->OnEndFrame(mDevice, mWindow);
	}
}
