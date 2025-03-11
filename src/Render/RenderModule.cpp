#include "Render/RenderPch.h"
#include "RenderModule.h"
#include "ScreenRenderer.h"
#include "WorldRenderer.h"
#include "RenderDoc/RenderDocIntegration.h"

#if defined(_DEBUG)
#define ENABLE_RENDER_DOC_PLUGIN 0
#else
#define ENABLE_RENDER_DOC_PLUGIN 0
#endif

RenderModule::RenderModule(GI::CreateGraphicsInfra* createGraphicsBackend)
	: mCreateGraphicsInfra(createGraphicsBackend)
{

}

void RenderModule::AdaptWindow(WindowType type, const Platform::WindowInfo& windowInfo, u8 frameCount)
{
	mWindows[type] = windowInfo;
	mGraphicInfra->AdaptToWindow(windowInfo, frameCount);
}


void RenderModule::OnResizeWindow(Platform::NativeWindowHandle windowHandle, const Vec2u& size)
{
	mFrameGraph->Unimport(mGraphicInfra->GetWindowBackBuffer(windowHandle));
	
	mGraphicInfra->ResizeWindow(windowHandle, size);
}

void RenderModule::Initial()
{
#if ENABLE_RENDER_DOC_PLUGIN
	mRenderDoc = new RenderDocIntegration;
#endif

	mGraphicInfra = mCreateGraphicsInfra();
	mFrameGraph = std::make_unique<FrameGraph>(mGraphicInfra);
}

void RenderModule::TickFrame(Timer* timer)
{
	PROFILE_EVENT(RenderModule::TickFrame);

	if (mScreenRenderer) { mScreenRenderer->TickFrame(timer); }
	if (mWorldRenderer) { mWorldRenderer->TickFrame(timer); }
	if (mImGuiRenderer) { mImGuiRenderer->TickFrame(timer); }
}

void RenderModule::Render()
{
	PROFILE_EVENT(RenderModule::Render);

	if (mRenderDoc)
	{
		mRenderDoc->OnStartFrame(mGraphicInfra->GetNativeDevicePtr(), mWindows[WindowType::MainPort].mNativeHandle);
	}

	mGraphicInfra->StartFrame();

	mFrameGraph->StartFrame();

	if (!mImGuiRenderer) { mImGuiRenderer = std::make_unique<ImGuiRenderer>(this); }
	if (!mScreenRenderer) { mScreenRenderer = std::make_unique<ScreenRenderer>(this); }
	if (!mWorldRenderer) { mWorldRenderer = std::make_unique<WorldRenderer>(this, mWindows[WindowType::MainPort].mSize); }

	{
		{
			PROFILE_EVENT(RenderToMainPort);

			auto sceneHdr = mWorldRenderer->Render();

			//RENDER_EVENT(mGraphicInfra, RenderToMainPort);

			const auto& backBuffer = mGraphicInfra->GetWindowBackBuffer(mWindows[WindowType::MainPort].mNativeHandle);
			auto target = mFrameGraph->Import(backBuffer);
			mScreenRenderer->Render(sceneHdr, target);
			mImGuiRenderer->Render(target, mUiData);
			mFrameGraph->Present(target);
		}

		{
			PROFILE_EVENT(RenderToDebugPort);

			const auto& backBuffer = mGraphicInfra->GetWindowBackBuffer(mWindows[WindowType::DebugPort].mNativeHandle);
			auto target = mFrameGraph->Import(backBuffer);
			mWorldRenderer->RenderGBufferChannels(target);
			mWorldRenderer->RenderShadowMaskChannel(target);
			mWorldRenderer->RenderLightViewDepthChannel(target);
			mFrameGraph->Present(target);
		}
	}

	mFrameGraph->EndFrame();

	mGraphicInfra->EndFrame();
	
	mGraphicInfra->Present();

	if (mRenderDoc)
	{
		mRenderDoc->OnEndFrame(mGraphicInfra->GetNativeDevicePtr(), mWindows[WindowType::MainPort].mNativeHandle);
	}
}

void RenderModule::Destroy()
{
	mScreenRenderer = nullptr;
	mWorldRenderer = nullptr;
	mImGuiRenderer = nullptr;

	mFrameGraph = nullptr;

	Utils::SafeDelete(mGraphicInfra);
}
