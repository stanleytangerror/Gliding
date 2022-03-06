#pragma once

#include <chrono>
#include "CommonTypes.h"

class GD_COMMON_API Timer
{
public:
	Timer();

	void OnStartNewFrame();

	f32 GetCurrentFrameElapsedSeconds() const;
	f32 GetLastFrameDeltaTime() const;

protected:
	using DurationMS = std::chrono::duration<f32, std::milli>;
	using DurationS = std::chrono::duration<f32>;
	using TimePoint = std::chrono::steady_clock::time_point;

	const TimePoint	mStartTimePoint;
	TimePoint		mCurFrameStartTimePoint;
	DurationS		mLastFrameDuration;
};

