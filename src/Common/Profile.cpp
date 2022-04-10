#include "CommonPch.h"
#include "Profile.h"
#include "StringUtils.h"

#define MICROPROFILE_IMPL
#include "microprofile.h"

void GD_COMMON_API Profile::Initial()
{
	MicroProfileOnThreadCreate("Main");
	MicroProfileSetForceEnable(true);
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);
	MicroProfileWebServerStart();
}

void GD_COMMON_API Profile::Flush()
{
	MicroProfileFlip();
}

void GD_COMMON_API Profile::Destroy()
{
	MicroProfileShutdown();
}

#define MICROPROFILE_ENTERI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MICROPROFILE_INVALID_TOKEN; if(MICROPROFILE_INVALID_TOKEN == MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__)){MicroProfileGetTokenC(&MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), group, name, color, MicroProfileTokenTypeCpu);} MicroProfileEnter(MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))

Profile::ScopedCpuEvent::ScopedCpuEvent(const char* name)
{
	mToken = MicroProfileGetToken("Main", name, 0xff0000ff, MicroProfileTokenTypeCpu);
	mTick = MicroProfileEnter(mToken);
}

Profile::ScopedCpuEvent::~ScopedCpuEvent()
{
	MicroProfileLeave(mToken, mTick);
}
