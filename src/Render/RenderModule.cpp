#include "RenderPch.h"
#include "RenderModule.h"
#include "ScreenRenderer.h"
#include "WorldRenderer.h"
#include "RenderDoc/RenderDocIntegration.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12RenderTarget.h"
#include "D3D12/D3D12SwapChain.h"

RenderModule::RenderModule(WindowInfo windowInfo)
	: mWindowInfo(windowInfo)
{
	//mRenderDoc = new RenderDocIntegration;

	mDevice = new D3D12Device(mWindowInfo.mWindow, mWindowInfo.mSize);
	const auto& backBuffer = mDevice->GetBackBuffer()->GetBuffer();
	mBackBufferSize = { backBuffer->GetWidth(), backBuffer->GetHeight() };

	mScreenRenderer = std::make_unique<ScreenRenderer>(this);
	mWorldRenderer = std::make_unique<WorldRenderer>(this);

	mSceneHdrRt = new D3D12RenderTarget(mDevice, mDevice->GetBackBuffer()->GetBuffer()->GetSize(), DXGI_FORMAT_R11G11B10_FLOAT, "HdrRt");
}

void RenderModule::TickFrame(Timer* timer)
{
	if (mRenderDoc)
	{
		mRenderDoc->OnStartFrame(mDevice, mWindowInfo.mWindow);
	}

	mDevice->StartFrame();

	mScreenRenderer->TickFrame(timer);
	mWorldRenderer->TickFrame(timer);

	GraphicsContext* context = mDevice->GetGpuQueue(D3D12GpuQueueType::Graphic)->AllocGraphicContext();
	{
		SwapChainBufferResource* backBuffer = mDevice->GetBackBuffer()->GetBuffer();
		
		mWorldRenderer->Render(context, mSceneHdrRt->GetRtv());
		mScreenRenderer->Render(context, mSceneHdrRt->GetSrv(), backBuffer->GetRtv());

		{
			RENDER_EVENT(context, DebugChannels);
			mWorldRenderer->RenderGBufferChannels(context, backBuffer->GetRtv());
			mWorldRenderer->RenderShadowMaskChannel(context, backBuffer->GetRtv());
			mWorldRenderer->RenderLightViewDepthChannel(context, backBuffer->GetRtv());
		}

		backBuffer->Transition(context, D3D12_RESOURCE_STATE_PRESENT);
	}
	context->Finalize();

	mDevice->Present();

	if (mRenderDoc)
	{
		mRenderDoc->OnEndFrame(mDevice, mWindowInfo.mWindow);
	}
}

void RenderModule::Destroy()
{
	mScreenRenderer = nullptr;
	mWorldRenderer = nullptr;
	Utils::SafeDelete(mSceneHdrRt);

	mDevice->Destroy();
	Utils::SafeDelete(mDevice);
}
