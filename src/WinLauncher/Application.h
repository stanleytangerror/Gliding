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
	Application();

	void			Initial(u32 width, u32 height, std::string name, HINSTANCE hInstance, int nCmdShow);
	void			Destroy();

	void			Run();
	void			RunLogic();

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static HWND CreateWindowInner(u32 width, u32 height, std::string name, HINSTANCE hInstance, int nCmdShow);

	HWND							mWindowHandle = {};

	std::unique_ptr<Timer>			mTimer;
	std::unique_ptr<RenderModule>	mRenderModule;

	std::unique_ptr<std::thread>	mLogicThread;

	enum class AppLifeCycle { Initial, Running, Destroying };
	std::atomic<AppLifeCycle> mAppLifeCycle = AppLifeCycle::Initial;
};