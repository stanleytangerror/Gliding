#pragma once

#include <wtypes.h>

class RenderModule;

extern "C"
{
	GD_RENDER_API RenderModule* CreateRenderModule();
}