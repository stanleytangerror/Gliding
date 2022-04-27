#include "RenderPch.h"
#include "RenderModule.h"
#include "ScreenRenderer.h"
#include "WorldRenderer.h"
#include "RenderDoc/RenderDocIntegration.h"
#include "D3D12Backend/D3D12Device.h"
#include "D3D12Backend/D3D12RenderTarget.h"
#include "D3D12Backend/D3D12SwapChain.h"

#if defined(_DEBUG)
#define ENABLE_RENDER_DOC_PLUGIN 0
#else
#define ENABLE_RENDER_DOC_PLUGIN 0
#endif

RenderModule::RenderModule()
{
#if ENABLE_RENDER_DOC_PLUGIN
	mRenderDoc = new RenderDocIntegration;
#endif

	mDevice = new D3D12Device;
}

void RenderModule::AdaptWindow(PresentPortType type, const WindowInfo& windowInfo)
{
	mDevice->CreateSwapChain(type, HWND(windowInfo.mWindow), windowInfo.mSize);
}

void RenderModule::Initial()
{
	const Vec3i& mainPortBackBufferSize = mDevice->GetPresentPort(PresentPortType::MainPort).mSwapChain->GetBuffer()->GetSize();
	const Vec2i& mainPortSize = { mainPortBackBufferSize.x(), mainPortBackBufferSize.y() };

	mScreenRenderer = std::make_unique<ScreenRenderer>(this);
	mWorldRenderer = std::make_unique<WorldRenderer>(this, mainPortSize);

	mSceneHdrRt = new D3D12RenderTarget(mDevice, mainPortBackBufferSize, DXGI_FORMAT_R11G11B10_FLOAT, "HdrRt");
}

void RenderModule::TickFrame(Timer* timer)
{
	PROFILE_EVENT(RenderModule::TickFrame);

	if (mRenderDoc)
	{
		mRenderDoc->OnStartFrame(mDevice, HWND(mDevice->GetPresentPort(PresentPortType::MainPort).mWindow));
	}

	mDevice->StartFrame();

	{
		PROFILE_EVENT(RendererLogic);

		mScreenRenderer->TickFrame(timer);
		mWorldRenderer->TickFrame(timer);
	}

	GraphicsContext* context = mDevice->GetGpuQueue(D3D12GpuQueueType::Graphic)->AllocGraphicContext();
	{
		{
			RENDER_EVENT(context, RenderWorldToHdr);
			mWorldRenderer->Render(context, mSceneHdrRt->GetRtv());
		}

		{
			RENDER_EVENT(context, RenderToMainPort);
		
			SwapChainBufferResource* backBuffer = mDevice->GetPresentPort(PresentPortType::MainPort).mSwapChain->GetBuffer();
			mScreenRenderer->Render(context, mSceneHdrRt->GetSrv(), backBuffer->GetRtv());

			backBuffer->Transition(context, D3D12_RESOURCE_STATE_PRESENT);
		}

		{
			RENDER_EVENT(context, DebugChannels);
			
			SwapChainBufferResource* backBuffer = mDevice->GetPresentPort(PresentPortType::DebugPort).mSwapChain->GetBuffer();
			mWorldRenderer->RenderGBufferChannels(context, backBuffer->GetRtv());
			mWorldRenderer->RenderShadowMaskChannel(context, backBuffer->GetRtv());
			mWorldRenderer->RenderLightViewDepthChannel(context, backBuffer->GetRtv());
		
			backBuffer->Transition(context, D3D12_RESOURCE_STATE_PRESENT);
		}
	}
	context->Finalize();

	mDevice->Present();

	if (mRenderDoc)
	{
		mRenderDoc->OnEndFrame(mDevice, HWND(mDevice->GetPresentPort(PresentPortType::MainPort).mWindow));
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
