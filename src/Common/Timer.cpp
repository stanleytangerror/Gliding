#include "CommonPch.h"
#include "Timer.h"

Timer::Timer()
	: mStartTimePoint(std::chrono::steady_clock::now())
{

}

void Timer::OnStartNewFrame()
{
	TimePoint lastFrameStart = mCurFrameStartTimePoint;
	TimePoint curFrameStart = std::chrono::steady_clock::now();

	mCurFrameStartTimePoint = curFrameStart;
	mLastFrameDuration = curFrameStart - lastFrameStart;
}

f32 Timer::GetCurrentFrameElapsedSeconds() const
{
	return (DurationS(mCurFrameStartTimePoint - mStartTimePoint)).count();
}

f32 Timer::GetLastFrameDeltaTime() const
{
	return mLastFrameDuration.count();
}

