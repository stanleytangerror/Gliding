#include "ImGuiIntegration.h"
#include <windows.h>
#include "backends/imgui_impl_win32.h"

IMGUI_INTEGRATION_API bool ImGuiIntegration::Initial()
{
	return ImGui::CreateContext() != nullptr;
}

bool ImGuiIntegration::AttachToWindow(const u64 windowHandle)
{
	return ImGui_ImplWin32_Init(HWND(windowHandle));
}

void ImGuiIntegration::BeginUI()
{
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

ImDrawData* ImGuiIntegration::EndUI()
{
	if (ImGui::GetIO().Fonts->TexID)
	{
		ImGui::Render();
	}
	ImGui::EndFrame();
	return ImGui::GetDrawData();
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
