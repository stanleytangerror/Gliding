#include "WinLauncherPch.h"
#include "Application.h"
#include "Common/PresentPort.h"
#include "ImGuiIntegration/ImGuiIntegration.h"
#include "Render/WorldRenderer.h"
#include <mutex>

struct WinMessage
{
	HWND hWnd = 0;
	UINT message = 0;
	WPARAM wParam = 0;
	LPARAM lParam = 0;
};
static std::vector<WinMessage>			sMessages;
static std::mutex						sMessageMutex;

std::vector<WinMessage> ReadMessages()
{
	std::lock_guard<std::mutex> guard(sMessageMutex);
	std::vector<WinMessage> result;
	std::swap(result, sMessages);
	return result;
}

void WriteMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::lock_guard<std::mutex> guard(sMessageMutex);
	sMessages.push_back(WinMessage{ hWnd, message, wParam, lParam });
}

Application::Application()
	: mTimer(std::make_unique<Timer>())
{
	Profile::Initial();
	mRenderModule = std::make_unique<RenderModule>();
	ImGuiIntegration::Initial();
}

void Application::Initial(HINSTANCE hInstance, int nCmdShow)
{
	mLogicThread = std::make_unique<std::thread>([this]() { this->LogicThread(); });
	mWindowThread = std::make_unique<std::thread>([&]() { this->WindowThread(hInstance, nCmdShow); });
}

void Application::Destroy()
{
	ImGuiIntegration::Shutdown();
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

	ImGuiIntegration::AttachToWindow(mMainWindowInfo.mWindow);

	mRenderModule->AdaptWindow(PresentPortType::MainPort, mMainWindowInfo);
	mRenderModule->AdaptWindow(PresentPortType::DebugPort, mDebugWindowInfo);
	mRenderModule->Initial();

	while (mMainWindowInfo.mWindow != 0 && mDebugWindowInfo.mWindow != 0)
	{
		mTimer->OnStartNewFrame();

		for (const WinMessage& msg : ReadMessages())
		{
			ImGuiIntegration::WindowProcHandler(u64(msg.hWnd), msg.message, msg.wParam, msg.lParam);
		}

		ImGuiIntegration::BeginUI();
		{
			const auto& fullWindowSize = mMainWindowInfo.mSize;

			bool open = true;
			ImGui::SetNextWindowPos({});
			ImGui::SetNextWindowSize(ImGui::FromVec2(fullWindowSize));
			ImGui::Begin("OperatePanel", &open,
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_NoTitleBar);
			{
				static Vec2f lastDragInScreenSpace = Vec2f::Zero();
				const Vec2f curDragInScreenSpace = ImGui::ToVec2<f32>(ImGui::GetMouseDragDelta(ImGuiMouseButton_Left));

				Vec2f deltaDragInScreenSpace;
				if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				{
					deltaDragInScreenSpace = Vec2f::Zero();
					lastDragInScreenSpace = Vec2f::Zero();
				}
				else
				{
					deltaDragInScreenSpace = curDragInScreenSpace - lastDragInScreenSpace;
					lastDragInScreenSpace = curDragInScreenSpace;
				}

				/* +x: camera right, +y: camera down */
				const Vec3f dragInViewSpace = Vec3f(deltaDragInScreenSpace.x(), -deltaDragInScreenSpace.y(), 0.f) / std::min<f32>(f32(fullWindowSize.x()), f32(fullWindowSize.y()));

				if (!Math::AlmostZero(dragInViewSpace))
				{
					WorldRenderer* worldRenderer = mRenderModule->GetWorldRenderer();
					Math::CameraTransformf& camTrans = worldRenderer->mCameraTrans;
					
					const Vec3f dragInWorldSpace =
						dragInViewSpace.x() * camTrans.CamRightInWorldSpace() +
						dragInViewSpace.y() * camTrans.CamUpInWorldSpace();
					
					const Rotationf rotInWorldSpace = Math::FromAngleAxis<f32>(
						dragInViewSpace.norm() * Math::Pi<f32>() * 2.f, 
						dragInWorldSpace.cross(camTrans.CamDirInWorldSpace()).normalized());

					worldRenderer->mTestModel->mRelTransform = Transformf(rotInWorldSpace) * worldRenderer->mTestModel->mRelTransform;
				}
			}
			ImGui::End();
		}
		ImDrawData* uiDate = ImGuiIntegration::EndUI();
		mRenderModule->mUiData = uiDate;

		mRenderModule->TickFrame(mTimer.get());

		mRenderModule->Render();

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

	case WM_PAINT:
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
	Assert(AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE));

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

	Assert(windowHandle != 0);

	Assert(SetWindowText(windowHandle, name.c_str()));

	ShowWindow(windowHandle, nCmdShow);

	return windowHandle;
}
