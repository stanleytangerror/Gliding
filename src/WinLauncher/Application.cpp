#include "WinLauncherPch.h"
#include "Application.h"
#include "Common/PresentPort.h"

Application::Application()
	: mTimer(std::make_unique<Timer>())
{
	Profile::Initial();
	mRenderModule = std::make_unique<RenderModule>();
}

void Application::Initial(HINSTANCE hInstance, int nCmdShow)
{
	mLogicThread = std::make_unique<std::thread>([this]() { this->LogicThread(); });
	mWindowThread = std::make_unique<std::thread>([&]() { this->WindowThread(hInstance, nCmdShow); });
}

void Application::Destroy()
{
	mAppLifeCycle = AppLifeCycle::Destroying;
	mRenderModule->Destroy();
	Profile::Destroy();
}

void Application::Run()
{
	mAppLifeCycle = AppLifeCycle::Running;
	mWindowThread->join();
	mLogicThread->join();
}

void Application::LogicThread()
{
	// https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex
	AssertHResultOk(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED));

	while (!mWindowCreated) {}

	mRenderModule->AdaptWindow(PresentPortType::MainPort, mMainWindowInfo);
	mRenderModule->AdaptWindow(PresentPortType::DebugPort, mDebugWindowInfo);
	mRenderModule->Initial();

	while (mMainWindowInfo.mWindow != 0 && mDebugWindowInfo.mWindow != 0)
	{
		mTimer->OnStartNewFrame();
		
		mRenderModule->TickFrame(mTimer.get());

		Profile::Flush();
	}
}

void Application::WindowThread(HINSTANCE hInstance, int nCmdShow)
{
	mMainWindowInfo.mSize = { 1600, 900 };
	mMainWindowInfo.mWindow = PortHandle(CreateWindowInner(1600, 900, "MainWindow", hInstance, nCmdShow));

	mDebugWindowInfo.mSize = { 640, 360 };
	mDebugWindowInfo.mWindow = PortHandle(CreateWindowInner(640, 360, "DebugWindow", hInstance, nCmdShow));

	mWindowCreated = true;

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

	mMainWindowInfo.mWindow = {};
	mDebugWindowInfo.mWindow = {};
}

LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
	return 0;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_MOUSEMOVE:
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDOWN:
	{
	}
	return 0;

	case WM_PAINT:
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND Application::CreateWindowInner(u32 width, u32 height, std::string name, HINSTANCE hInstance, int nCmdShow)
{
	// Initialize the window class.
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = "WinLauncher";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	HWND windowHandle = CreateWindow(
		windowClass.lpszClassName,
		name.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,		// We have no parent window.
		nullptr,		// We aren't using menus.
		hInstance,
		nullptr);

	SetWindowText(windowHandle, name.c_str());

	ShowWindow(windowHandle, nCmdShow);

	return windowHandle;
}
