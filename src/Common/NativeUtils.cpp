#include "CommonPch.h"
#include "NativeUtils.h"

GD_COMMON_API Timer* CreateTimer()
{
    return new Timer;
}

GD_COMMON_API void TimerOnStartNewFrame(Timer* timer)
{
    timer->OnStartNewFrame();
}
