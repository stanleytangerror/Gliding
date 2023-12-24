#pragma once

#include "windows.h"
#include "WorldRenderer.h"
#include "ScreenRenderer.h"
#include "ImGuiRenderer.h"
#include "Common/PresentPort.h"
#include "Common/GraphicsInfrastructure.h"
#include "imgui.h"

namespace GI
{
	class IGraphicInfra;
}
class ScreenRenderer;
class RenderDocIntegration;
class WorldRenderer;
namespace D3D12Backend
{
	class RenderTarget;
	class D3D12Device;
}
class ImGuiRenderer;

class GD_RENDER_API RenderModule
{
public:
	RenderModule();

	void AdaptWindow(PresentPortType type, const WindowRuntimeInfo& windowInfo);

	void Initial();

	void TickFrame(Timer* timer);
	void Render();

	D3D12Backend::D3D12Device*	GetDevice() const { return mDevice; }
	GI::IGraphicInfra*			GetGraphicsInfra() const { return mGraphicInfra; }
	WorldRenderer*				GetWorldRenderer() const { return mWorldRenderer.get(); }

	void				Destroy();

protected:
	GI::IGraphicInfra*						mGraphicInfra = nullptr;
	D3D12Backend::D3D12Device*				mDevice = nullptr;
	RenderDocIntegration*					mRenderDoc = nullptr;

	std::unique_ptr<ScreenRenderer>			mScreenRenderer;
	std::unique_ptr<WorldRenderer>			mWorldRenderer;
	std::unique_ptr<ImGuiRenderer>			mImGuiRenderer;

	RenderTarget*		mSceneHdrRt = nullptr;

public:
	ImDrawData*								mUiData = nullptr;
};

