#include "RenderPch.h"
#include "RenderModule.h"
#include "ScreenRenderer.h"
#include "WorldRenderer.h"
#include "RenderDoc/RenderDocIntegration.h"

#if defined(_DEBUG)
#define ENABLE_RENDER_DOC_PLUGIN 0
#else
#define ENABLE_RENDER_DOC_PLUGIN 0
#endif

RenderModule::RenderModule(CreateGraphicsInfra* createGraphicsBackend)
	: mCreateGraphicsInfra(createGraphicsBackend)
{

}

void RenderModule::AdaptWindow(PresentPortType type, const WindowRuntimeInfo& windowInfo)
{
	mWindowInfo[type] = windowInfo;
	mGraphicInfra->AdaptToWindow(u8(type), windowInfo);
}


void RenderModule::OnResizeWindow(u8 windowId, const Vec2u& size)
{
	mGraphicInfra->ResizeWindow(windowId, size);
}

void RenderModule::Initial(const Vec2u& initialSize)
{
#if ENABLE_RENDER_DOC_PLUGIN
	mRenderDoc = new RenderDocIntegration;
#endif

	mGraphicInfra = mCreateGraphicsInfra();
	mFrameGraph = std::make_unique<FrameGraph>(mGraphicInfra);

	mGraphicInfra->StartRecording();

	mScreenRenderer = std::make_unique<ScreenRenderer>(this);
	mWorldRenderer = std::make_unique<WorldRenderer>(this, initialSize);
	mImGuiRenderer = std::make_unique<ImGuiRenderer>(this);

	mGraphicInfra->EndRecording(false);
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
	PROFILE_EVENT(RenderModule::Render);

	if (mRenderDoc)
	{
		mRenderDoc->OnStartFrame(mGraphicInfra->GetNativeDevicePtr(), mWindowInfo[PresentPortType::MainPort].mNativeHandle);
	}

	mGraphicInfra->StartFrame();

	mFrameGraph->StartFrame();

	{
		{
			PROFILE_EVENT(RenderToMainPort);

			auto sceneHdr = mWorldRenderer->Render();

			//RENDER_EVENT(mGraphicInfra, RenderToMainPort);

			const auto& backBuffer = mGraphicInfra->GetWindowBackBuffer(u8(PresentPortType::MainPort));
			auto target = mFrameGraph->Import(backBuffer);
			mScreenRenderer->Render(sceneHdr, target);
			mImGuiRenderer->Render(target, mUiData);
			mFrameGraph->Present(target);
		}

		{
			PROFILE_EVENT(RenderToDebugPort);

			const auto& backBuffer = mGraphicInfra->GetWindowBackBuffer(u8(PresentPortType::DebugPort));
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
		mRenderDoc->OnEndFrame(mGraphicInfra->GetNativeDevicePtr(), mWindowInfo[PresentPortType::MainPort].mNativeHandle);
	}
}

void RenderModule::Destroy()
{
	mScreenRenderer = nullptr;
	mWorldRenderer = nullptr;
	mImGuiRenderer = nullptr;

	Utils::SafeDelete(mGraphicInfra);
}
