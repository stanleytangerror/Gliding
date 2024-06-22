#include "RenderPch.h"
#include "RenderModule.h"
#include "ScreenRenderer.h"
#include "WorldRenderer.h"
#include "RenderDoc/RenderDocIntegration.h"
#include "RenderTarget.h"

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

	mSceneHdrRt = std::make_unique<RenderTarget>(mGraphicInfra, Vec3u{ initialSize.x(), initialSize.y(), 1 }, GI::Format::FORMAT_R11G11B10_FLOAT, "HdrRt");

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
	if (mRenderDoc)
	{
		mRenderDoc->OnStartFrame(mGraphicInfra->GetNativeDevicePtr(), mWindowInfo[PresentPortType::MainPort].mNativeHandle);
	}

	mGraphicInfra->StartFrame();

	mFrameGraph->StartFrame();

	auto sceneHdr = mFrameGraph->Import(mSceneHdrRt->GetResource());
	{
		{
			RtvUsageFuture sceneHdrRtv = { sceneHdr, mSceneHdrRt->GetRtvDesc() };
			RENDER_EVENT(mGraphicInfra, RenderWorldToHdr);
			mWorldRenderer->Render(mGraphicInfra, sceneHdrRtv);
		}

		{
			RENDER_EVENT(mGraphicInfra, RenderToMainPort);

			const auto& backBuffer = mGraphicInfra->GetWindowBackBuffer(u8(PresentPortType::MainPort));
			auto target = RtvUsageFuture {
				mFrameGraph->Import(backBuffer),
				GI::RtvDesc()
					.SetFormat(backBuffer->GetFormat())
					.SetViewDimension(GI::RtvDimension::TEXTURE2D)
					.SetTexture2D_MipSlice(0)
					.SetTexture2D_PlaneSlice(0) };
			mScreenRenderer->Render(mGraphicInfra, SrvUsageFuture{ sceneHdr, mSceneHdrRt->GetSrvDesc() }, target);
			mImGuiRenderer->Render(mGraphicInfra, target, mUiData);
			mFrameGraph->Present(target.resource);
		}

		{
			RENDER_EVENT(mGraphicInfra, DebugChannels);

			const auto& backBuffer = mGraphicInfra->GetWindowBackBuffer(u8(PresentPortType::DebugPort));
			auto target = RtvUsageFuture{
				mFrameGraph->Import(backBuffer),
				GI::RtvDesc()
					.SetFormat(backBuffer->GetFormat())
					.SetViewDimension(GI::RtvDimension::TEXTURE2D)
					.SetTexture2D_MipSlice(0)
					.SetTexture2D_PlaneSlice(0) };
			mWorldRenderer->RenderGBufferChannels(mGraphicInfra, target);
			mWorldRenderer->RenderShadowMaskChannel(mGraphicInfra, target);
			mWorldRenderer->RenderLightViewDepthChannel(mGraphicInfra, target);
			mFrameGraph->Present(target.resource);
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
	mSceneHdrRt = nullptr;
	mImGuiRenderer = nullptr;

	Utils::SafeDelete(mGraphicInfra);
}
