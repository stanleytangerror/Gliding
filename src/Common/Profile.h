#pragma once

namespace Profile
{
	void GD_COMMON_API Initial();
	void GD_COMMON_API Flush();

	class GD_COMMON_API ScopedCpuEvent
	{
	public:
		ScopedCpuEvent(const char* name);
		~ScopedCpuEvent();
	};
}

#define PROFILE_EVENT(name)				Profile::ScopedCpuEvent _Event_##_FILE_##_LINE_NO_(#name);