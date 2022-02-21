#pragma once

#include "Common/CommonTypes.h"
#include <xstring>
#include <thread>
#include <wtypes.h>
#include "Render/RenderModule.h"

class Application
{
public:
	Application(u32 width, u32 height, std::string name, HINSTANCE hInstance, int nCmdShow);

	void			Initial();

	void			Run();
	void			RunLogic();

	u32 GetWidth() const { return mWidth; }
	u32 GetHeight() const { return mHeight; }
	const char* GetName() const { return mTitle.c_str(); }

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// Viewport dimensions.
	u32 mWidth;
	u32 mHeight;
	const std::string mTitle;
	HWND				mWindowHandle = {};

	RenderModule* mRenderModule = nullptr;

	std::thread*		mLogicThread = nullptr;
};