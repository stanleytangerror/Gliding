#pragma once

#include "WinGuiMacros.h"
#include "Common/CommonTypes.h"
#include "Common/PresentPort.h"
#include <map>
#include <mutex>
#include <wchar.h>
#include <xstring>

namespace WinGui
{
	struct Message
	{
		u64 hWnd;
		u64 message;
		u64 wParam;
		u64 lParam;
	};

	struct WindowCreationInfo
	{
		std::wstring title;
		Vec2i size;
	};

	class GuiSystem
	{
	public:
		GuiSystem();

		void CreateNewWindow(const WindowCreationInfo& info);

		void PeakAllMessages();
		bool CanDequeueMessage() const;
		Message DequeueMessage();

	private:
		std::mutex mMutex;
		std::vector<Message> mMessages;

		std::mutex mWindowManageMutex;
		std::unique_ptr<std::thread> mWindowThread;
		std::vector<WindowCreationInfo> mCreateWindowQueue;
		std::map<u64, WindowCreationInfo> mWindowHandles;


	};
}

extern "C"
{
	WINGUI_API WinGui::GuiSystem* CreateWinGuiSystem();
	WINGUI_API void CreateNewGuiWindow(WinGui::GuiSystem* system, const wchar_t* title, const Vec2i& size);
	
	WINGUI_API void FlushMessages(WinGui::GuiSystem* system);
	WINGUI_API bool DequeueMessage(WinGui::GuiSystem* system, WinGui::Message* message);
}