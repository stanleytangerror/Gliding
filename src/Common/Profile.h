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
		ScopedCpuEvent(u64 token);
		~ScopedCpuEvent();

		static u64 GetToken(const char* name);

	private:
		u64 mToken = 0;
		u64 mTick = 0;
	};
}

#define PROFILE_EVENT(name)	\
	static const u64 _Profile_ScopedCpuEvent_##_FILE_##_LINE_NO_##_Token = Profile::ScopedCpuEvent::GetToken(#name); \
	Profile::ScopedCpuEvent _Profile_ScopedCpuEvent_##_FILE_##_LINE_NO_(_Profile_ScopedCpuEvent_##_FILE_##_LINE_NO_##_Token);

//#include "zeux-microprofile/microprofile.h"
//#define PROFILE_EVENT(name)				MICROPROFILE_SCOPEI("group",#name, MP_YELLOW);