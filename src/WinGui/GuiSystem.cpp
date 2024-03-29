#include "pch.h"
#include "GuiSystem.h"
#include <mutex>
#include "Common/StringUtils.h"

namespace WinGui
{
	class WindowMessageQueue
	{
	private:
		std::vector<Message> mMessages;
		std::mutex			 mMessageMutex;

	public:
		std::vector<Message> ReadMessages()
		{
			std::lock_guard<std::mutex> guard(mMessageMutex);
			std::vector<Message> result;
			std::swap(result, mMessages);
			return result;
		}

		void WriteMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			std::lock_guard<std::mutex> guard(mMessageMutex);
			mMessages.push_back(Message{ u64(hWnd), message, wParam, u64(lParam) });
		}

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
				WriteMessage(hWnd, message, wParam, lParam);
			}

			// Handle any messages the switch statement didn't.
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	};

	static WindowMessageQueue sMessageHub;

	HWND CreateWindowInner(UINT width, UINT height, const wchar_t* windowTitle)
	{
		HINSTANCE hInstance = GetModuleHandle(NULL);

		// Initialize the window class.
		WNDCLASSEX windowClass = { 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		const WNDPROC proc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { return sMessageHub.WindowProc(hWnd, message, wParam, lParam); };
		windowClass.lpfnWndProc = proc;
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
				this->mWindowThreadId = std::this_thread::get_id();

				MSG msg = {};
				while (msg.message != WM_QUIT)
				{
					// process window creation queue
					{
						std::lock_guard<std::mutex> guard(mWindowManageMutex);

						for (const auto& info : this->mCreateWindowQueue)
						{
							const u64 handle = PortHandle(CreateWindowInner(info.mSize.x(), info.mSize.y(), info.mTitle.c_str()));
							mWindowMap[info.mWindowId] = WindowRuntimeInfo{ handle, info.mSize };
						}
						this->mCreateWindowQueue.clear();
					}
					
					// Process any messages in the queue.
					if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			});
	}

	WindowId GuiSystem::CreateNewWindow(const wchar_t* title, const Vec2i& size)
	{
		std::lock_guard<std::mutex> guard(mWindowManageMutex);
		
		const WindowId windowId = mWindowIdAllocator.Alloc();
		mCreateWindowQueue.push_back(WindowCreationInfo{ title, size, windowId });

		return windowId;
	}

	bool GuiSystem::TryGetWindowInfo(const WindowId& windowId, WindowRuntimeInfo* info)
	{
		std::lock_guard<std::mutex> guard(mWindowManageMutex);
		
		auto it = mWindowMap.find(windowId);
		if (it != mWindowMap.end())
		{
			*info = it->second;
			return true;
		}
		
		return false;
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

WINGUI_API WinGui::WindowId CreateNewGuiWindow(WinGui::GuiSystem* system, const wchar_t* title, const Vec2i& size)
{
	return system->CreateNewWindow(title, size);
}

WINGUI_API bool TryGetGuiWindowInfo(WinGui::GuiSystem* system, const WinGui::WindowId& id, WindowRuntimeInfo* info)
{
	return system->TryGetWindowInfo(id, info);
}
