#pragma once

#include "Common/CommonMacros.h"
#include "Common/CommonTypes.h"
#include "Eigen/Eigen"
#include "Common/Math.h"

#ifdef ImGuiIntegrationExport
#define IMGUI_INTEGRATION_API __declspec(dllexport)
#else
#define IMGUI_INTEGRATION_API
#endif

#include "imgui.h"

namespace ImGuiIntegration
{
	IMGUI_INTEGRATION_API bool			Initial();
	IMGUI_INTEGRATION_API bool			AttachToWindow(const u64 windowHandle);
	IMGUI_INTEGRATION_API void			BeginUI();
	IMGUI_INTEGRATION_API ImDrawData*	EndUI();
	IMGUI_INTEGRATION_API void			Shutdown();

	IMGUI_INTEGRATION_API u64			WindowProcHandler(const u64 windowHandle, u32 msg, u64 wParam, u64 lParam);
}

namespace ImGui
{
	template <typename T>
	ImVec2 FromVec2(const Vec2<T>& v) { return { f32(v.x()), f32(v.y()) }; }

	template <typename T>
	ImVec4 FromVec4(const Vec4<T>& v) { return { f32(v.x()), f32(v.y()), f32(v.z()), f32(v.w()) }; }
}