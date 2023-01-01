#include "RenderPch.h"
#include "NativeUtils.h"
#include "RenderModule.h"

GD_RENDER_API RenderModule* CreateRenderModule()
{
	return new RenderModule();
}

