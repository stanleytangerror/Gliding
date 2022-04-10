#pragma once

#include "CommonTypes.h"

namespace Profile
{
	void GD_COMMON_API Initial();
	void GD_COMMON_API Flush();
	void GD_COMMON_API Destroy();

	class GD_COMMON_API ScopedCpuEvent
	{
	public:
		ScopedCpuEvent(const char* name);
		~ScopedCpuEvent();

	private:
		u64 mToken = 0;
		u64 mTick = 0;
	};
}

#define PROFILE_EVENT(name)				Profile::ScopedCpuEvent _Profile_ScopedCpuEvent_##_FILE_##_LINE_NO_(#name);