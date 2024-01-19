#include "RenderPch.h"
#include "NativeUtils.h"
#include "RenderModule.h"

GD_RENDER_API RenderModule* CreateRenderModule()
{
	return new RenderModule();
}

GD_RENDER_API void AdaptWindow(RenderModule* renderModule, PresentPortType type, const WindowRuntimeInfo& windowInfo)
{
	renderModule->AdaptWindow(type, windowInfo);
}

GD_RENDER_API void InitialRenderModule(RenderModule* renderModule)
{
	renderModule->Initial();
}

GD_RENDER_API void RenderThroughRenderModule(RenderModule* renderModule)
{
	renderModule->Render();
}

GD_RENDER_API void TickRenderModule(RenderModule* renderModule, Timer* timer)
{
	renderModule->TickFrame(timer);
}

