#include "RenderPch.h"
#include "RenderModule.h"
#include "ScreenRenderer.h"
#include "WorldRenderer.h"
#include "RenderDoc/RenderDocIntegration.h"
#include "D3D12Backend/D3D12Device.h"
#include "RenderTarget.h"
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

	mDevice = new D3D12Backend::D3D12Device;
}

void RenderModule::AdaptWindow(PresentPortType type, const WindowInfo& windowInfo)
{
	mDevice->CreateSwapChain(type, HWND(windowInfo.mWindow), windowInfo.mSize);
}

void RenderModule::Initial()
{
	const Vec3i& mainPortBackBufferSize = mDevice->GetPresentPort(PresentPortType::MainPort).mSwapChain->GetSize();
	const Vec2i& mainPortSize = { mainPortBackBufferSize.x(), mainPortBackBufferSize.y() };

	mScreenRenderer = std::make_unique<ScreenRenderer>(this);
	mWorldRenderer = std::make_unique<WorldRenderer>(this, mainPortSize);
	mImGuiRenderer = std::make_unique<ImGuiRenderer>(this);

	mSceneHdrRt = new RenderTarget(mDevice, mainPortBackBufferSize, DXGI_FORMAT_R11G11B10_FLOAT, "HdrRt");
}

void RenderModule::TickFrame(Timer* timer)
{
	PROFILE_EVENT(RenderModule::TickFrame);

	mScreenRenderer->TickFrame(timer);
	mWorldRenderer->TickFrame(timer);
	mImGuiRenderer->TickFrame(timer);
}

void RenderModule::Render()
{
	if (mRenderDoc)
	{
		mRenderDoc->OnStartFrame(mDevice, HWND(mDevice->GetPresentPort(PresentPortType::MainPort).mWindow));
	}

	mDevice->StartFrame();

	D3D12Backend::GraphicsContext* context = mDevice->GetGpuQueue(D3D12Backend::D3D12GpuQueueType::Graphic)->AllocGraphicContext();
	{
		{
			RENDER_EVENT(context, RenderWorldToHdr);
			mWorldRenderer->Render(context, mSceneHdrRt->GetRtv());
		}

		{
			RENDER_EVENT(context, RenderToMainPort);

			D3D12Backend::SwapChainBufferResource* backBuffer = mDevice->GetPresentPort(PresentPortType::MainPort).mSwapChain->GetBuffer();
			mScreenRenderer->Render(context, mSceneHdrRt->GetSrv(), backBuffer->GetRtv());
			mImGuiRenderer->Render(context, backBuffer->GetRtv(), mUiData);

			backBuffer->PrepareForPresent(context);
		}

		{
			RENDER_EVENT(context, DebugChannels);

			D3D12Backend::SwapChainBufferResource* backBuffer = mDevice->GetPresentPort(PresentPortType::DebugPort).mSwapChain->GetBuffer();
			mWorldRenderer->RenderGBufferChannels(context, backBuffer->GetRtv());
			mWorldRenderer->RenderShadowMaskChannel(context, backBuffer->GetRtv());
			mWorldRenderer->RenderLightViewDepthChannel(context, backBuffer->GetRtv());

			backBuffer->PrepareForPresent(context);
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
