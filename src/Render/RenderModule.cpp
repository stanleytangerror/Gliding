#include "RenderPch.h"
#include "RenderModule.h"
#include "RenderDoc/RenderDocIntegration.h"
#include "D3D12Backend/D3D12Device.h"
#include "D3D12Backend/D3D12SwapChain.h"
#include "RenderTarget.h"

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

	mResourceManager = std::make_unique<RenderResourceManager>(this);
	mRenderPassManager = std::make_unique<RenderPassManager>(this);
}

void RenderModule::AdaptWindow(PresentPortType type, const WindowRuntimeInfo& windowInfo)
{
	mDevice->CreateSwapChain(type, HWND(windowInfo.mNativeHandle), windowInfo.mSize);
}

void RenderModule::Initial()
{
	const Vec3i& mainPortBackBufferSize = mDevice->GetPresentPort(PresentPortType::MainPort).mSwapChain->GetSize();
	const Vec2i& mainPortSize = { mainPortBackBufferSize.x(), mainPortBackBufferSize.y() };

	mScreenRenderer = std::make_unique<ScreenRenderer>(this);
	mWorldRenderer = std::make_unique<WorldRenderer>(this, mainPortSize);
	mImGuiRenderer = std::make_unique<ImGuiRenderer>(this);
	mTestRenderer = std::make_unique<TestRenderer>(this);

	//mSceneHdrRt = new RenderTarget(mDevice, mainPortBackBufferSize, DXGI_FORMAT_R11G11B10_FLOAT, "HdrRt");

	mSceneHdrId = mResourceManager->CreateTransientResource(RHI::CommitedResourceDesc{
		RHI::ResourceDimention::Texture2D, 
		RHI::PixelFormat::R11G11B10_FLOAT, 
		RHI::ResourceSize{ (u64) mainPortBackBufferSize.x(), (u32) mainPortBackBufferSize.y(), (u16) mainPortBackBufferSize.z() },
		1});
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
		mRenderDoc->OnStartFrame(mDevice, HWND(mDevice->GetPresentPort(PresentPortType::MainPort).mNativeHandle));
	}

	mDevice->StartFrame();

	mResourceManager->PreRender();
	
	D3D12Backend::GraphicsContext* context = mDevice->GetGpuQueue(D3D12Backend::D3D12GpuQueueType::Graphic)->AllocGraphicContext();
	{
		mTestRenderer->TestRender(mRenderPassManager.get(), mResourceManager.get());
		mRenderPassManager->ParseAllPassses(context);

		//{
		//	RENDER_EVENT(context, RenderWorldToHdr);
		//	mWorldRenderer->Render(context, mSceneHdrRt->GetRtv());
		//}

		{
			RENDER_EVENT(context, RenderToMainPort);

			D3D12Backend::SwapChainBufferResource* backBuffer = mDevice->GetPresentPort(PresentPortType::MainPort).mSwapChain->GetBuffer();
			//mScreenRenderer->Render(context, mSceneHdrRt->GetSrv(), backBuffer->GetRtv());
			//mImGuiRenderer->Render(context, backBuffer->GetRtv(), mUiData);

			backBuffer->PrepareForPresent(context);
		}

		{
			RENDER_EVENT(context, DebugChannels);

			D3D12Backend::SwapChainBufferResource* backBuffer = mDevice->GetPresentPort(PresentPortType::DebugPort).mSwapChain->GetBuffer();
			//mWorldRenderer->RenderGBufferChannels(context, backBuffer->GetRtv());
			//mWorldRenderer->RenderShadowMaskChannel(context, backBuffer->GetRtv());
			//mWorldRenderer->RenderLightViewDepthChannel(context, backBuffer->GetRtv());

			backBuffer->PrepareForPresent(context);
		}
	}
	context->Finalize();

	mDevice->Present();

	if (mRenderDoc)
	{
		mRenderDoc->OnEndFrame(mDevice, HWND(mDevice->GetPresentPort(PresentPortType::MainPort).mNativeHandle));
	}
}

void RenderModule::Destroy()
{
	mScreenRenderer = nullptr;
	mWorldRenderer = nullptr;
	//Utils::SafeDelete(mSceneHdrRt);

	mDevice->Destroy();
	Utils::SafeDelete(mDevice);
}
