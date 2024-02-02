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

	using CreateGraphicsInfra = GI::IGraphicsInfra* ();

#ifdef _DEBUG
	mGraphicsBackendModule = LoadLibrary("D3D12Backend_Debug_x64.dll");
#else
	mGraphicsBackendModule = LoadLibrary("D3D12Backend_Release_x64.dll");
#endif

	// Get the function pointer
	auto createInfraFunc = reinterpret_cast<CreateGraphicsInfra*>(GetProcAddress(mGraphicsBackendModule, "CreateGraphicsInfra"));
	mGraphicInfra = createInfraFunc();

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
	{
		{
			RENDER_EVENT(mGraphicInfra, RenderWorldToHdr);
			mWorldRenderer->Render(mGraphicInfra, mSceneHdrRt->GetRtv());
		}

		{
			RENDER_EVENT(mGraphicInfra, RenderToMainPort);

			const auto& backBuffer = mGraphicInfra->GetWindowBackBuffer(u8(PresentPortType::MainPort));
			auto target = GI::RtvUsage(backBuffer);
			target
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
			auto target = GI::RtvUsage(backBuffer);
			target
				.SetFormat(backBuffer->GetFormat())
				.SetViewDimension(GI::RtvDimension::TEXTURE2D)
				.SetTexture2D_MipSlice(0)
				.SetTexture2D_PlaneSlice(0);
			mWorldRenderer->RenderGBufferChannels(mGraphicInfra, target);
			mWorldRenderer->RenderShadowMaskChannel(mGraphicInfra, target);
			mWorldRenderer->RenderLightViewDepthChannel(mGraphicInfra, target);
		}
	}
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

	FreeLibrary(mGraphicsBackendModule);
}
