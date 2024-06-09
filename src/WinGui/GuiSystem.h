#pragma once

#include "WinGuiMacros.h"
#include "Common/CommonTypes.h"
#include "Common/PresentPort.h"
#include "Common/IndexAllocator.h"
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

	struct WindowId
	{
		u64 mId;

		static WindowId FromInt(u64 id) { return { id }; }
		bool operator<(const WindowId& other) const { return mId < other.mId; }
	};

	struct WindowCreationInfo
	{
		std::wstring mTitle;
		Vec2u mSize;
		WindowId mWindowId;
	};

	class GuiSystem
	{
	public:
		GuiSystem();

		WindowId CreateNewWindow(const wchar_t* title, const Vec2u& size);
		bool TryGetWindowInfo(const WindowId& windowId, WindowRuntimeInfo* info);

		void PeakAllMessages();
		bool CanDequeueMessage() const;
		Message DequeueMessage();

	private:
		// main thread
		std::mutex mMutex;
		std::vector<Message> mMessages;
		IndexAllocator<WindowId> mWindowIdAllocator;

		// window thread
		std::mutex mWindowManageMutex;

		std::unique_ptr<std::thread> mWindowThread;
		std::atomic<std::thread::id> mWindowThreadId;
		
		// cross thread
		std::vector<WindowCreationInfo> mCreateWindowQueue;
		std::map<WindowId, WindowRuntimeInfo> mWindowMap;
	};
}

extern "C"
{
	WINGUI_API WinGui::GuiSystem* CreateWinGuiSystem();
	WINGUI_API WinGui::WindowId CreateNewGuiWindow(WinGui::GuiSystem* system, const wchar_t* title, const Vec2i& size);
	WINGUI_API bool TryGetGuiWindowInfo(WinGui::GuiSystem* system, const WinGui::WindowId& id, WindowRuntimeInfo* info);

	WINGUI_API void FlushMessages(WinGui::GuiSystem* system);
	WINGUI_API bool DequeueMessage(WinGui::GuiSystem* system, WinGui::Message* message);
}