#pragma once

#include "windows.h"
#include "WorldRenderer.h"
#include "ScreenRenderer.h"
#include "ImGuiRenderer.h"
#include "TestRender.h"
#include "RenderInterface/RenderResource.h"
#include "RenderInterface/RenderPass.h"
#include "Common/PresentPort.h"
#include "imgui.h"

class ScreenRenderer;
class RenderDocIntegration;
class WorldRenderer;
class ImGuiRenderer;
class RenderTarget;
namespace D3D12Backend
{
	class D3D12Device;
}

class GD_RENDER_API RenderModule
{
public:
	RenderModule();

	void AdaptWindow(PresentPortType type, const WindowRuntimeInfo& windowInfo);

	void Initial();

	void TickFrame(Timer* timer);
	void Render();

	D3D12Backend::D3D12Device*	GetDevice() const { return mDevice; }
	WorldRenderer*				GetWorldRenderer() const { return mWorldRenderer.get(); }
	RenderResourceManager*		GetRenderResourceManager() const { return mResourceManager.get(); }

	void				Destroy();

protected:
	D3D12Backend::D3D12Device*				mDevice = nullptr;
	RenderDocIntegration*					mRenderDoc = nullptr;
	std::unique_ptr<RenderResourceManager>	mResourceManager;
	std::unique_ptr<RenderPassManager>		mRenderPassManager;

	std::unique_ptr<ScreenRenderer>			mScreenRenderer;
	std::unique_ptr<WorldRenderer>			mWorldRenderer;
	std::unique_ptr<ImGuiRenderer>			mImGuiRenderer;
	std::unique_ptr<TestRenderer>			mTestRenderer;

	ResourceId								mSceneHdrId;

public:
	ImDrawData*								mUiData = nullptr;
};

