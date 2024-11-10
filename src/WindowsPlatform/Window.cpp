#include "Window.h"
#include <mutex>
#include "Common/StringUtils.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

namespace WindowsPlatform
{
	WindowsPlatform::WindowItem::WindowItem(const wchar_t* title, const Vec2u& initSize)
		: mTitle(title)
		, mInitSize(initSize)
	{
		mWindowThread = std::make_unique<std::thread>([this]() 
			{ 
				this->WindowThreadFunc(); 
			});
	}

	WindowItem::~WindowItem()
	{
		mState = State::eClosing;
		mWindowThread->join();
	}


	std::vector<WindowsPlatform::WindowItem::Message> WindowItem::ConsumeAllMessages()
	{
		std::vector<WindowsPlatform::WindowItem::Message> result;

		std::lock_guard<std::mutex> guard(mMessageMutex);
		std::swap(mMessages, result);
		return result;
	}

	Platform::WindowInfo WindowItem::GetInfo()
	{
		return Platform::WindowInfo{ mWindowHandle, mInitSize };
	}

	bool WindowItem::IsAlive()
	{
		return mState == State::eAlive;
	}

	void WindowItem::WindowThreadFunc()
	{
		static auto wndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT
		{
			switch (uMsg)
			{
			case WM_NCCREATE:
			{
				CREATESTRUCT* pCS = reinterpret_cast<CREATESTRUCT*>(lParam);
				LPVOID pThis = pCS->lpCreateParams;
				SetWindowLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(pThis));
				
				auto windowItem = reinterpret_cast<WindowItem*>(pThis);
				windowItem->mState |= State::eWindowProcessReady;
			}
			}

			if (WindowItem* windowItem = reinterpret_cast<WindowItem*>(GetWindowLongPtrW(hwnd, 0)))
			{
				windowItem->WindowProcess(uMsg, wParam, lParam);
			}

			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		};

		// https://stackoverflow.com/a/18162974/2131563
		HINSTANCE hInstance = GetModuleHandle(NULL);

		// Initialize the window class.
		WNDCLASSEX windowClass = { 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = wndProc;
		windowClass.cbWndExtra = sizeof(WindowItem*);
		windowClass.hInstance = hInstance;
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.lpszClassName = L"Test";
		RegisterClassEx(&windowClass);

		RECT windowRect = { 0, 0, static_cast<LONG>(mInitSize.x()), static_cast<LONG>(mInitSize.y()) };
		//AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		// Create the window and store a handle to it.
		HWND windowHandle = CreateWindow(
			windowClass.lpszClassName,
			mTitle.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,		// We have no parent window.
			nullptr,		// We aren't using menus.
			hInstance,
			//nullptr);
			static_cast<LPVOID>(this));

		SetWindowText(windowHandle, mTitle.c_str());

		int nCmdShow = SW_SHOWDEFAULT;
		ShowWindow(windowHandle, nCmdShow);

		mWindowHandle = u64(windowHandle);
		mState |= State::eNativeHandleReady;

		MSG msg = {};
		while (msg.message != WM_QUIT && msg.message != WM_DESTROY && mState == State::eAlive)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		DestroyWindow(windowHandle);
	}

	u64 WindowItem::WindowProcess(u64 message, u64 wParam, u64 lParam)
	{
		std::lock_guard<std::mutex> guard(mMessageMutex);
		mMessages.emplace_back(message, wParam, lParam);
		return 0;
	}

}

Platform::IWindow* CreateNativeWindow(const wchar_t* title, const Vec2u& initSize)
{
	return new WindowsPlatform::WindowItem(title, initSize);
}

WINDOWSPLATFORM_API void DestroyNativeWindow(Platform::IWindow* window)
{
	if (window)
	{
		delete window;
	}
}
