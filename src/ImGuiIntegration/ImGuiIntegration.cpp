#include "ImGuiIntegration.h"
#include <windows.h>

#define IMGUI_API __declspec(dllexport)
#include "imgui.h"
#include "backends/imgui_impl_win32.h"

IMGUI_INTEGRATION_API bool ImGuiIntegration::Initial()
{
	return ImGui::CreateContext() != nullptr;
}

bool ImGuiIntegration::AttachToWindow(const u64 windowHandle)
{
	return ImGui_ImplWin32_Init(HWND(windowHandle));
}

void ImGuiIntegration::OnNewFrame()
{
	ImGui_ImplWin32_NewFrame();
}

void ImGuiIntegration::Shutdown()
{
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

u64 ImGuiIntegration::WindowProcHandler(const u64 windowHandle, u32 msg, u64 wParam, u64 lParam)
{
	return ImGui_ImplWin32_WndProcHandler(HWND(windowHandle), msg, wParam, lParam);
}
