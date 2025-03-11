#pragma once

#include "windows.h"
#include "WorldRenderer.h"
#include "ScreenRenderer.h"
#include "ImGuiRenderer.h"
#include "Common/Platform.h"
#include "Common/GraphicsInfrastructure.h"
#include "imgui.h"
#include "FrameGraph.h"

class ScreenRenderer;
class RenderDocIntegration;
class WorldRenderer;
class ImGuiRenderer;

enum class GD_COMMON_API WindowType : u8
{
	MainPort = 0,
	DebugPort = 1,
};

class GD_RENDER_API RenderModule
{
public:
	RenderModule(GI::CreateGraphicsInfra* createGraphicsBackend);

	void AdaptWindow(WindowType type, const Platform::WindowInfo& windowInfo, u8 frameCount);
	void OnResizeWindow(Platform::NativeWindowHandle windowHandle, const Vec2u& size);

	void Initial();

	void TickFrame(Timer* timer);
	void Render();

	FrameGraph*					GetFrameGraph() const { return mFrameGraph.get(); }
	GI::IGraphicsInfra*			GetGraphicsInfra() const { return mGraphicInfra; }
	WorldRenderer*				GetWorldRenderer() const { return mWorldRenderer.get(); }
	ImGuiRenderer*				GetImGuiRenderer() const { return mImGuiRenderer.get(); }

	void				Destroy();

protected:
	GI::CreateGraphicsInfra*				mCreateGraphicsInfra = nullptr;
	GI::IGraphicsInfra*						mGraphicInfra = nullptr;
	RenderDocIntegration*					mRenderDoc = nullptr;

	std::unique_ptr<FrameGraph>				mFrameGraph;
	std::unique_ptr<ScreenRenderer>			mScreenRenderer;
	std::unique_ptr<WorldRenderer>			mWorldRenderer;
	std::unique_ptr<ImGuiRenderer>			mImGuiRenderer;

	std::map<WindowType, Platform::WindowInfo> mWindows;

public:
	ImDrawData*								mUiData = nullptr;
};

