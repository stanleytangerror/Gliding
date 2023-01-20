#pragma once

#include <wtypes.h>
#include "Common/PresentPort.h"

class RenderModule;

extern "C"
{
	GD_RENDER_API RenderModule* CreateRenderModule();

	GD_RENDER_API void AdaptWindow(RenderModule* renderModule, PresentPortType type, const WindowRuntimeInfo& windowInfo);

	GD_RENDER_API void InitialRenderModule(RenderModule* renderModule);
	GD_RENDER_API void RenderThroughRenderModule(RenderModule* renderModule);
	GD_RENDER_API void TickRenderModule(RenderModule* renderModule, Timer* timer);
}