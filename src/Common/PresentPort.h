#pragma once

#include "Math.h"

using PortHandle = u64;

enum class GD_COMMON_API PresentPortType
{
	MainPort = 0,
	DebugPort = 1,
};

struct GD_COMMON_API WindowRuntimeInfo
{
	PortHandle		mNativeHandle = {};
	Vec2i			mSize = {};
};
