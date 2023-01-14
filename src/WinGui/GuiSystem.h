#pragma once

#include "WinGuiMacros.h"
#include "Common/CommonTypes.h"
#include "Common/PresentPort.h"

namespace WinGui
{
	struct Message
	{
		u64 hWnd;
		u64 message;
		u64 wParam;
		u64 lParam;
	};

	class GuiSystem
	{
	public:
		GuiSystem();

		void PeakAllMessages();
		bool CanDequeueMessage() const;
		Message DequeueMessage();

	private:
		WindowInfo	mMainWindowInfo = {};
		WindowInfo	mDebugWindowInfo = {};
		std::vector<Message> mMessages;

		std::unique_ptr<std::thread>	mWindowThread;
	};
}

extern "C"
{
	WINGUI_API WinGui::GuiSystem* CreateWinGuiSystem();
	WINGUI_API PortHandle CreateNewWindow(WinGui::GuiSystem* system, const char* name);
	
	WINGUI_API void FlushMessages(WinGui::GuiSystem* system);
	WINGUI_API bool DequeueMessage(WinGui::GuiSystem* system, WinGui::Message* message);
}