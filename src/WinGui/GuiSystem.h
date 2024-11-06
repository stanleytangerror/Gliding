#pragma once

#include "WinGuiMacros.h"
#include "Common/CommonTypes.h"
#include "Common/Math.h"
#include <mutex>
#include <wchar.h>
#include <xstring>

namespace WinGui
{
	class WINGUI_API WindowItem
	{
	public:
		enum class State
		{
			eInitial, eActive, eClosing
		};

		struct Message
		{
			u64 message;
			u64 wParam;
			u64 lParam;
		};

	public:
		WindowItem(const wchar_t* title, const Vec2u& initSize);
		virtual ~WindowItem();

	private:
		void WindowThreadFunc();
		u64 WindowProcess(u64 message, u64 wParam, u64 lParam);

	private:
		std::wstring					mTitle;
		Vec2u							mInitSize;
		std::unique_ptr<std::thread>	mWindowThread;
		std::atomic<u64>				mWindowHandle = 0;
		std::atomic<State>				mState = State::eInitial;

		// window thread
		std::mutex				mMessageMutex;
		std::vector<Message>	mMessages;
	};

}
