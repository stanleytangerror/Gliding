#pragma once

#include "Common/CommonTypes.h"
#include "Render/RenderModule.h"
#include <xstring>
#include <thread>
#include <wtypes.h>
#include <memory>

class Timer;

class Application
{
public:
	Application(u32 width, u32 height, std::string name, HINSTANCE hInstance, int nCmdShow);

	void			Initial();
	void			Destroy();

	void			Run();
	void			RunLogic();

	u32				GetWidth() const { return mWidth; }
	u32				GetHeight() const { return mHeight; }
	const char*		GetName() const { return mTitle.c_str(); }

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	u32 mWidth;
	u32 mHeight;
	const std::string mTitle;
	HWND				mWindowHandle = {};

	std::unique_ptr<Timer>	mTimer;
	std::unique_ptr<RenderModule> mRenderModule;

	std::unique_ptr<std::thread>	mLogicThread;

	enum class AppLifeCycle { Initial, Running, Destroying };
	std::atomic<AppLifeCycle> mAppLifeCycle = AppLifeCycle::Initial;
};