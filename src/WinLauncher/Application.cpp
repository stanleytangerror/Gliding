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

class MouseDrag
{
public:
	MouseDrag(ImGuiMouseButton mouseButton) : mMouseButton(mouseButton) {}

	void Update()
	{
		const Vec2f curDragInPixelSpace = ImGui::ToVec2<f32>(ImGui::GetMouseDragDelta(mMouseButton, 0.1f));

		if (ImGui::IsMouseReleased(mMouseButton))
		{
			mDeltaDragInPixelSpace = Vec2f::Zero();
			mLastDragInPixelSpace = Vec2f::Zero();
		}
		else
		{
			mDeltaDragInPixelSpace = curDragInPixelSpace - mLastDragInPixelSpace;
			mLastDragInPixelSpace = curDragInPixelSpace;
		}
	}

	Vec2f	GetDragDeltaInPixelSpace() const { return mDeltaDragInPixelSpace; }

protected:
	ImGuiMouseButton	mMouseButton = ImGuiMouseButton_Left;
	Vec2f				mLastDragInPixelSpace = Vec2f::Zero();
	Vec2f				mDeltaDragInPixelSpace = Vec2f::Zero();
};

void Application::LogicThread()
{
	// https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex
	AssertHResultOk(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED));

	while (!mWindowCreated) {}

	ImGuiIntegration::AttachToWindow(mMainWindowInfo.mNativeHandle);

	mRenderModule->AdaptWindow(PresentPortType::MainPort, mMainWindowInfo);
	mRenderModule->AdaptWindow(PresentPortType::DebugPort, mDebugWindowInfo);
	mRenderModule->Initial();

	while (mMainWindowInfo.mNativeHandle != 0 && mDebugWindowInfo.mNativeHandle != 0)
	{
		mTimer->OnStartNewFrame();

		for (const WinMessage& msg : ReadMessages())
		{
			if (msg.message == WM_SIZE)
			{
				UINT width = LOWORD(msg.lParam);
				UINT height = HIWORD(msg.lParam);
				u8 windowId = (mMainWindowInfo.mNativeHandle == PortHandle(msg.hWnd)) ? u8(PresentPortType::MainPort) : u8(PresentPortType::DebugPort);
				mRenderModule->OnResizeWindow(windowId, { width, height });
				DEBUG_PRINT("WM_SIZE %d, size %d %d", msg.wParam, width, height);
			}

			ImGuiIntegration::WindowProcHandler(u64(msg.hWnd), msg.message, msg.wParam, msg.lParam);
		}

		ImGuiIntegration::BeginUI();
		{
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
					static std::array<MouseDrag, 2> drags = {
						MouseDrag(ImGuiMouseButton_Left),
						MouseDrag(ImGuiMouseButton_Right) };

					for (auto& drag : drags)
					{
						drag.Update();
					}

					WorldRenderer* worldRenderer = mRenderModule->GetWorldRenderer();
					Math::CameraTransformf& camTrans = worldRenderer->mCameraTrans;
					{
						/* +x: camera right, +y: camera down */
						const Vec2f leftButtonDeltaDragInPixelSpace = drags[ImGuiMouseButton_Left].GetDragDeltaInPixelSpace();
						const Vec3f dragInViewSpace = Vec3f(leftButtonDeltaDragInPixelSpace.x(), -leftButtonDeltaDragInPixelSpace.y(), 0.f) / std::min<f32>(f32(fullWindowSize.x()), f32(fullWindowSize.y()));

						if (!Math::AlmostZero(dragInViewSpace))
						{

							const Vec3f dragInWorldSpace =
								dragInViewSpace.x() * camTrans.CamRightInWorldSpace() +
								dragInViewSpace.y() * camTrans.CamUpInWorldSpace();

							const Rotationf rotInWorldSpace = Math::FromAngleAxis<f32>(
								dragInViewSpace.norm() * Math::Pi<f32>() * 2.f,
								dragInWorldSpace.cross(camTrans.CamDirInWorldSpace()).normalized());

							worldRenderer->mTestModel->mRelTransform = Transformf(rotInWorldSpace) * worldRenderer->mTestModel->mRelTransform;
						}
					}

					{
						const Vec2f rightButtonDeltaDragInPixelSpace = drags[ImGuiMouseButton_Right].GetDragDeltaInPixelSpace();
						const f32 camRotDeltaRad = Math::DegreeToRadian(rightButtonDeltaDragInPixelSpace.x() / fullWindowSize.y() * 360.f);

						const Vec3f lastCamDir = camTrans.CamDirInWorldSpace();
						const f32 camRotRad = std::atan2f(lastCamDir.x(), lastCamDir.y()) + camRotDeltaRad;

						const Vec3f& camDir = Vec3f{ std::sin(camRotRad), std::cos(camRotRad), 0.f };
						const Vec3f& camUp = Math::Axis3DDir<f32>(Math::Axis3D_Zp);
						const Vec3f& camRight = camDir.cross(camUp);

						camTrans.AlignCamera(camDir, camUp, camRight);
						camTrans.MoveCamera(-100.f * camDir);
					}
				}
				ImGui::End();
			}

			{
				bool open = true;
				ImGui::Begin("Debug UI", &open);
				{
					ImGui::Text("Debugging...");
				}
				ImGui::End();
			}
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
	mMainWindowInfo.mNativeHandle = PortHandle(CreateWindowInner(1600, 900, "MainWindow", hInstance, nCmdShow));

	mDebugWindowInfo.mSize = { 640, 360 };
	mDebugWindowInfo.mNativeHandle = PortHandle(CreateWindowInner(640, 360, "DebugWindow", hInstance, nCmdShow));

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

	mMainWindowInfo.mNativeHandle = {};
	mDebugWindowInfo.mNativeHandle = {};
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
