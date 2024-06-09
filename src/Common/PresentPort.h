#pragma once

#include "Math.h"

using PortHandle = u64;

enum class GD_COMMON_API PresentPortType : u8
{
	MainPort = 0,
	DebugPort = 1,
};

struct GD_COMMON_API WindowRuntimeInfo
{
	PortHandle		mNativeHandle = {};
	Vec2u			mSize = {};
	u32				mFrameCount = 3;
};
