#include "RenderPch.h"
#include "RenderModule.h"
#include "ScreenRenderer.h"
#include "WorldRenderer.h"
#include "RenderDoc/RenderDocIntegration.h"
#include "RenderTarget.h"
#include "D3D12Backend/D3D12GraphicsInfra.h"

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

	mGraphicInfra = new D3D12Backend::D3D12GraphicsInfra();
}

void RenderModule::AdaptWindow(PresentPortType type, const WindowRuntimeInfo& windowInfo)
{
	mWindowInfo[type] = windowInfo;
	mGraphicInfra->AdaptToWindow(u8(type), windowInfo);
}


void RenderModule::OnResizeWindow(u8 windowId, const Vec2i& size)
{
	mGraphicInfra->ResizeWindow(windowId, size);
	mSyncMode = true;
}

void RenderModule::Initial()
{
	const Vec2i& mainPortBackBufferSize = mWindowInfo[PresentPortType::MainPort].mSize;

	mScreenRenderer = std::make_unique<ScreenRenderer>(this);
	mWorldRenderer = std::make_unique<WorldRenderer>(this, mainPortBackBufferSize);
	mImGuiRenderer = std::make_unique<ImGuiRenderer>(this);

	mSceneHdrRt = new RenderTarget(mGraphicInfra, { mainPortBackBufferSize.x(), mainPortBackBufferSize.y(), 1 }, GI::Format::FORMAT_R11G11B10_FLOAT, "HdrRt");
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
	{
		{
			RENDER_EVENT(mGraphicInfra, RenderWorldToHdr);
			mWorldRenderer->Render(mGraphicInfra, mSceneHdrRt->GetRtv());
		}

		{
			RENDER_EVENT(mGraphicInfra, RenderToMainPort);

			const auto& backBuffer = mGraphicInfra->GetWindowBackBuffer(u8(PresentPortType::MainPort));
			const auto& target = GI::RtvDesc()
				.SetResource(backBuffer)
				.SetFormat(backBuffer->GetFormat())
				.SetViewDimension(GI::RtvDimension::TEXTURE2D)
				.SetTexture2D_MipSlice(0)
				.SetTexture2D_PlaneSlice(0);
			mScreenRenderer->Render(mGraphicInfra, mSceneHdrRt->GetSrv(), target);
			mImGuiRenderer->Render(mGraphicInfra, target, mUiData);
		}

		{
			RENDER_EVENT(mGraphicInfra, DebugChannels);

			const auto& backBuffer = mGraphicInfra->GetWindowBackBuffer(u8(PresentPortType::DebugPort));
			const auto& target = GI::RtvDesc()
				.SetResource(backBuffer)
				.SetFormat(backBuffer->GetFormat())
				.SetViewDimension(GI::RtvDimension::TEXTURE2D)
				.SetTexture2D_MipSlice(0)
				.SetTexture2D_PlaneSlice(0);
			mWorldRenderer->RenderGBufferChannels(mGraphicInfra, target);
			mWorldRenderer->RenderShadowMaskChannel(mGraphicInfra, target);
			mWorldRenderer->RenderLightViewDepthChannel(mGraphicInfra, target);
		}
	}
	mGraphicInfra->EndFrame(mSyncMode);
	
	mSyncMode = false;

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
	Utils::SafeDelete(mSceneHdrRt);
}
