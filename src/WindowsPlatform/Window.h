#pragma once

#include "WindowsPlatformMacros.h"
#include "Common/CommonTypes.h"
#include "Common/CommonMath.h"
#include "Common/Platform.h"
#include <mutex>
#include <wchar.h>
#include <xstring>

namespace WindowsPlatform
{
	class WINDOWSPLATFORM_API WindowItem : public Platform::IWindow
	{
	public:
		WindowItem(const wchar_t* title, const Vec2u& initSize);
		virtual ~WindowItem();

		std::vector<Platform::Message>	ConsumeAllMessages() override;
		Platform::WindowInfo	GetInfo() override;
		bool					IsAlive() override;

	private:
		void WindowThreadFunc();
		u64 WindowProcess(u64 message, u64 wParam, u64 lParam);

	private:
		std::wstring					mTitle;
		Vec2u							mInitSize;
		std::unique_ptr<std::thread>	mWindowThread;
		std::atomic<u64>				mWindowHandle = 0;
		std::atomic<u8>					mState = State::eInitial;

		// window thread
		std::mutex				mMessageMutex;
		std::vector<Platform::Message>	mMessages;
	};
}

extern "C"
{
	WINDOWSPLATFORM_API Platform::IWindow* CreateNativeWindow(const wchar_t* title, const Vec2u& initSize);
	WINDOWSPLATFORM_API void DestroyNativeWindow(Platform::IWindow* window);
}