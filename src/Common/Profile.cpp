#include "CommonPch.h"
#include "Profile.h"
#include "StringUtils.h"
#include "microprofile.h"

void GD_COMMON_API Profile::Initial()
{
#if MICROPROFILE_ENABLED
	MicroProfileOnThreadCreate("Main");
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);
	MicroProfileStartContextSwitchTrace();
#endif
}

void GD_COMMON_API Profile::Flush()
{
#if MICROPROFILE_ENABLED
	MicroProfileFlip(nullptr);
#endif
}

void GD_COMMON_API Profile::Destroy()
{
#if MICROPROFILE_ENABLED
	MicroProfileShutdown();
#endif
}

Profile::ScopedCpuEvent::ScopedCpuEvent(const char* name)
{
	MICROPROFILE_ENTERI("Main", name, 0xff0000ff);
}

Profile::ScopedCpuEvent::~ScopedCpuEvent()
{
	MICROPROFILE_LEAVE();
}
