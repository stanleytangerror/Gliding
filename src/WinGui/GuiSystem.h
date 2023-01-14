#pragma once

#include "WinGuiMacros.h"
#include "Common/CommonTypes.h"
#include "Common/PresentPort.h"

namespace WinGui
{
	class GuiSystem
	{
	public:
		GuiSystem();
		void Run();

	private:
		WindowInfo	mMainWindowInfo = {};
		WindowInfo	mDebugWindowInfo = {};
	};
}

extern "C"
{
	WINGUI_API WinGui::GuiSystem* CreateWinGuiSystem();
	WINGUI_API void UpdateWinGuiSystem(WinGui::GuiSystem* system);
	WINGUI_API PortHandle CreateNewWindow(WinGui::GuiSystem* system, const char* name);
}