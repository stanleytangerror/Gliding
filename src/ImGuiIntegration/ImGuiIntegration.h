#pragma once

#include "Common/CommonMacros.h"
#include "Common/CommonTypes.h"
#include "Common/Platform.h"
#include "Eigen/Eigen"
#include "Common/CommonMath.h"

#ifdef ImGuiIntegrationExport
#define IMGUI_INTEGRATION_API __declspec(dllexport)
#else
#define IMGUI_INTEGRATION_API
#endif

#include "imgui.h"

extern "C"
{
	namespace ImGuiIntegration
	{
		IMGUI_INTEGRATION_API bool			Initial();
		IMGUI_INTEGRATION_API bool			AttachToWindow(Platform::NativeWindowHandle windowHandle);
		IMGUI_INTEGRATION_API void			BeginUI();
		IMGUI_INTEGRATION_API ImDrawData*	EndUI();
		IMGUI_INTEGRATION_API void			Shutdown();

		IMGUI_INTEGRATION_API u64			WindowProcHandler(Platform::NativeWindowHandle windowHandle, Platform::Message message);
	}
}

namespace ImGui
{
	template <typename T>
	ImVec2 FromVec2(const Vec2<T>& v) { return { f32(v.x()), f32(v.y()) }; }

	template <typename T>
	Vec2<T> ToVec2(const ImVec2& v) { return { T(v.x), T(v.y) }; }

	template <typename T>
	ImVec4 FromVec4(const Vec4<T>& v) { return { f32(v.x()), f32(v.y()), f32(v.z()), f32(v.w()) }; }

	template <typename T>
	Vec4<T> ToVec4(const ImVec4& v) { return { T(v.x), T(v.y), T(v.z), T(v.w) }; }
}