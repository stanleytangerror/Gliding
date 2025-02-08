#pragma once

#include "ApplicationHeader.h"
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

	void			Initial( HINSTANCE hInstance, int nCmdShow);
	void			Destroy();

	void			Run();

protected:
	void			LogicFrame(bool& continueLoop);
	void			HandleMessages(bool& continueLoop);
	void			AddGui();

	HMODULE							mGraphicsBackendModule = {};
	HMODULE							mPlatformModule = {};

	Platform::IWindow*				mMainWindow = nullptr;
	Platform::IWindow*				mDebugWindow = nullptr;

	std::unique_ptr<Timer>			mTimer;
	std::unique_ptr<RenderModule>	mRenderModule;

	enum class AppLifeCycle { Initial, Running, Destroying };
	std::atomic<AppLifeCycle> mAppLifeCycle = AppLifeCycle::Initial;
};