#pragma once

#include <wtypes.h>
#include "Common/PresentPort.h"

class RenderModule;

extern "C"
{
	GD_RENDER_API RenderModule* CreateRenderModule();

	GD_RENDER_API void AdaptWindow(RenderModule* renderModule, const WindowRuntimeInfo& windowInfo);
}