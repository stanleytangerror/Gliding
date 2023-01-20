#pragma once

#include "CommonMacros.h"
#include "Timer.h"

extern "C"
{
	GD_COMMON_API Timer* CreateTimer();

	GD_COMMON_API void TimerOnStartNewFrame(Timer* timer);
}