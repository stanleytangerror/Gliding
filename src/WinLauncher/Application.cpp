#include "WinLauncher/WinLauncherPch.h"
#include "Application.h"
#include "Common/Platform.h"
#include "Common/Math.h"
#include "Common/Platform.h"
#include "ImGuiIntegration/ImGuiIntegration.h"
#include "Render/WorldRenderer.h"
#include <mutex>
#include <map>

Application::Application()
	: mTimer(std::make_unique<Timer>())
{
#ifdef _DEBUG
	mGraphicsBackendModule = LoadLibrary("D3D12Backend_Debug_x64.dll");
	mPlatformModule = LoadLibrary("WindowsPlatform_Debug_x64.dll");
#else
	mGraphicsBackendModule = LoadLibrary("D3D12Backend_Release_x64.dll");
	mPlatformModule = LoadLibrary("WindowsPlatform_Release_x64.dll");
#endif

	// https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex
	AssertHResultOk(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED));

	Profile::Initial();

	auto createInfraFunc = reinterpret_cast<GI::CreateGraphicsInfra*>(GetProcAddress(mGraphicsBackendModule, "CreateGraphicsInfra"));
	mRenderModule = std::make_unique<RenderModule>(createInfraFunc);
	ImGuiIntegration::Initial();
}

void Application::Initial(HINSTANCE hInstance, int nCmdShow)
{
	auto createNativeWindow = reinterpret_cast<Platform::CreateNativeWindow*>(GetProcAddress(mPlatformModule, "CreateNativeWindow"));
	auto destroyNativeWindow = reinterpret_cast<Platform::DestroyNativeWindow*>(GetProcAddress(mPlatformModule, "DestroyNativeWindow"));

	mMainWindow = createNativeWindow(L"MainWindow", Vec2u{ 1600, 900 });
	mDebugWindow = createNativeWindow(L"DebugWindow", Vec2u{ 640, 360 });

	while (!mMainWindow->IsAlive() || !mDebugWindow->IsAlive()) {};

	ImGuiIntegration::AttachToWindow(mMainWindow->GetInfo().mNativeHandle);

	mRenderModule->Initial();

	mRenderModule->AdaptWindow(WindowType::MainPort, mMainWindow->GetInfo(), 3);
	mRenderModule->AdaptWindow(WindowType::DebugPort, mDebugWindow->GetInfo(), 3);
}

void Application::Destroy()
{
	ImGuiIntegration::Shutdown();
	mAppLifeCycle = AppLifeCycle::Destroying;
	mRenderModule->Destroy();
	Profile::Destroy();

	FreeLibrary(mGraphicsBackendModule);
}

void Application::Run()
{
	mAppLifeCycle = AppLifeCycle::Running;

	while (true)
	{
		LogicFrame();
	}
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

void Application::LogicFrame()
{
	PROFILE_EVENT(Application::LogicFrame);

	mTimer->OnStartNewFrame();
	DEBUG_PRINT(" ===================== Frame no %lld, last frame duration %f ======================== ", mTimer->GetFrameNo(), mTimer->GetLastFrameDeltaTime());

	const auto& messageProcess = [this](Platform::IWindow* window)
	{
		const auto& windowInfo = window->GetInfo();
		for (const auto& msg : window->ConsumeAllMessages())
		{
			if (msg.message == WM_SIZE)
			{
				UINT width = LOWORD(msg.lParam);
				UINT height = HIWORD(msg.lParam);
				const auto& newSize = Vec2u{ width, height };

				this->mRenderModule->OnResizeWindow(windowInfo.mNativeHandle, newSize);
				DEBUG_PRINT("Window %d size (%d, %d)", windowInfo.mNativeHandle, newSize.x(), newSize.y());
			}

			ImGuiIntegration::WindowProcHandler(windowInfo.mNativeHandle, msg.message, msg.wParam, msg.lParam);
		}
	};

	messageProcess(mMainWindow);
	messageProcess(mDebugWindow);

	if (mRenderModule->GetImGuiRenderer())
	{
		ImGuiIntegration::BeginUI();
		{
			{
				const auto& fullWindowSize = mMainWindow->GetInfo().mSize;

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

					Math::CameraTransformf& camTrans = mRenderModule->GetFrameGraph()->GetBlackboard()->Get<MainCameraState>().mCameraTrans;
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
	}

	mRenderModule->TickFrame(mTimer.get());

	mRenderModule->Render();

	Profile::Flush();
}
