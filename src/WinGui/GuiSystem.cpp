#include "pch.h"
#include "GuiSystem.h"
#include <mutex>

namespace WinGui
{
	class MessageHub
	{
	private:
		std::vector<Message> sMessages;
		std::mutex			sMessageMutex;

	public:
		std::vector<Message> ReadMessages()
		{
			std::lock_guard<std::mutex> guard(sMessageMutex);
			std::vector<Message> result;
			std::swap(result, sMessages);
			return result;
		}

		void WriteMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			std::lock_guard<std::mutex> guard(sMessageMutex);
			sMessages.push_back(Message{ u64(hWnd), message, wParam, u64(lParam) });
		}
	};

	static MessageHub sMessageHub;

	LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CREATE:
		{
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
		return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		default:
			sMessageHub.WriteMessage(hWnd, message, wParam, lParam);
		}

		// Handle any messages the switch statement didn't.
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	HWND CreateWindowInner(UINT width, UINT height, const wchar_t* windowTitle)
	{
		HINSTANCE hInstance = GetModuleHandle(NULL);

		// Initialize the window class.
		WNDCLASSEX windowClass = { 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = hInstance;
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.lpszClassName = L"WinLauncher";
		RegisterClassEx(&windowClass);

		RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		// Create the window and store a handle to it.
		HWND windowHandle = CreateWindow(
			windowClass.lpszClassName,
			windowTitle,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,		// We have no parent window.
			nullptr,		// We aren't using menus.
			hInstance,
			nullptr);

		SetWindowText(windowHandle, windowTitle);

		int nCmdShow = SW_SHOWDEFAULT;
		ShowWindow(windowHandle, nCmdShow);

		return windowHandle;
	}

	GuiSystem::GuiSystem()
	{
		mWindowThread = std::make_unique<std::thread>([&]() 
			{ 
				mMainWindowInfo.mWindow = PortHandle(CreateWindowInner(1600, 900, L"MainWindow"));

				mDebugWindowInfo.mSize = { 640, 360 };
				mDebugWindowInfo.mWindow = PortHandle(CreateWindowInner(640, 360, L"DebugWindow"));

				MSG msg = {};
				while (msg.message != WM_QUIT)
				{
					// Process any messages in the queue.
					if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			});
		
	}

	void GuiSystem::PeakAllMessages()
	{
		const auto& newMessages = sMessageHub.ReadMessages();
		mMessages.insert(mMessages.end(), newMessages.begin(), newMessages.end());
	}

	bool GuiSystem::CanDequeueMessage() const
	{
		return !mMessages.empty();
	}

	WinGui::Message GuiSystem::DequeueMessage()
	{
		auto msg = mMessages.front();
		mMessages.erase(mMessages.begin());
		return msg;
	}
}

WinGui::GuiSystem* CreateWinGuiSystem()
{
	return new WinGui::GuiSystem;
}

PortHandle CreateNewWindow(WinGui::GuiSystem* system, const char* name)
{
	return 0;
}

WINGUI_API bool DequeueMessage(WinGui::GuiSystem* system, WinGui::Message* message)
{
	if (system->CanDequeueMessage())
	{
		*message = system->DequeueMessage();
		return true;
	}

	return false;
}

WINGUI_API void FlushMessages(WinGui::GuiSystem* system)
{
	system->PeakAllMessages();
}
