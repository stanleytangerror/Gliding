#include "RenderPch.h"
#include "NativeUtils.h"
#include "RenderModule.h"

GD_RENDER_API RenderModule* CreateRenderModule()
{
	return new RenderModule();
}

GD_RENDER_API void AdaptWindow(RenderModule* renderModule, const WindowRuntimeInfo& windowInfo)
{
	renderModule->AdaptWindow(PresentPortType::MainPort, windowInfo);
}

