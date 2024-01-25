#pragma once

#include "windows.h"
#include "WorldRenderer.h"
#include "ScreenRenderer.h"
#include "ImGuiRenderer.h"
#include "Common/PresentPort.h"
#include "Common/GraphicsInfrastructure.h"
#include "imgui.h"

class ScreenRenderer;
class RenderDocIntegration;
class WorldRenderer;
class ImGuiRenderer;

class GD_RENDER_API RenderModule
{
public:
	RenderModule();

	void AdaptWindow(PresentPortType type, const WindowRuntimeInfo& windowInfo);
	void OnResizeWindow(u8 windowId, const Vec2u& size);

	void Initial();

	void TickFrame(Timer* timer);
	void Render();

	GI::IGraphicsInfra*			GetGraphicsInfra() const { return mGraphicInfra; }
	WorldRenderer*				GetWorldRenderer() const { return mWorldRenderer.get(); }

	void				Destroy();

protected:
	GI::IGraphicsInfra*						mGraphicInfra = nullptr;
	RenderDocIntegration*					mRenderDoc = nullptr;

	std::unique_ptr<ScreenRenderer>			mScreenRenderer;
	std::unique_ptr<WorldRenderer>			mWorldRenderer;
	std::unique_ptr<ImGuiRenderer>			mImGuiRenderer;

	RenderTarget*							mSceneHdrRt = nullptr;

	std::map<PresentPortType, WindowRuntimeInfo> mWindowInfo;
	bool									mSyncMode = false;

public:
	ImDrawData*								mUiData = nullptr;
};

