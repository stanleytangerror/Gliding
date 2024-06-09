#include "CommonPch.h"
#include "Profile.h"
#include "StringUtils.h"
#include "AssertUtils.h"

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

Profile::ScopedCpuEvent::ScopedCpuEvent(u64 token)
	: mToken(token)
{
	mTick = MicroProfileEnter(mToken);
}

Profile::ScopedCpuEvent::~ScopedCpuEvent()
{
	MicroProfileLeave(mToken, mTick);
}

u64 Profile::ScopedCpuEvent::GetToken(const char* name)
{
	// microprofile does not support name with `"`, see errors in browser (F12)
	Assert(std::strchr(name, '"') == nullptr); 

	const u32 color = Utils::HashBytes(reinterpret_cast<const std::byte*>(name), strlen(name));
	return MicroProfileGetToken("Main", name, color, MicroProfileTokenTypeCpu);
}
