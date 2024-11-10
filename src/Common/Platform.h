#pragma once

#include "CommonTypes.h"
#include "Math.h"

namespace Platform
{
	using NativeWindowHandle = u64;

	struct GD_COMMON_API WindowInfo
	{
		NativeWindowHandle	mNativeHandle = {};
		Vec2u				mSize = {};
	};

	struct Message
	{
		u64 message;
		u64 wParam;
		u64 lParam;
	};

	class GD_COMMON_API IWindow
	{
	public:
		enum State : u8
		{
			eInitial = 0, 
			eNativeHandleReady = 0b01,
			eWindowProcessReady = 0b10,
			eAlive = 0b11, 
			eClosing = 0b100
		};

		virtual std::vector<Message>	ConsumeAllMessages() = 0;
		virtual WindowInfo				GetInfo() = 0;
		virtual bool					IsAlive() = 0;
	};

	typedef Platform::IWindow* CreateNativeWindow(const wchar_t* title, const Vec2u& initSize);
	typedef void DestroyNativeWindow(Platform::IWindow*);
}
