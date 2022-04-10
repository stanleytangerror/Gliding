#include "CommonPch.h"
#include "Profile.h"
#include "microprofile.h"

void GD_COMMON_API Profile::Initial()
{
#if MICROPROFILE_ENABLED
	MicroProfileSetEnableAllGroups(true);
#endif
}

void GD_COMMON_API Profile::Flush()
{
#if MICROPROFILE_ENABLED
	MicroProfileFlip(nullptr);
#endif
}

Profile::ScopedCpuEvent::ScopedCpuEvent(const char* name)
{
	MICROPROFILE_ENTERI("MicroProfile", name, 0);
}

Profile::ScopedCpuEvent::~ScopedCpuEvent()
{
	MICROPROFILE_LEAVE();
}
