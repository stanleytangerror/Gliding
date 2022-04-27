#pragma once

#include "Math.h"

using PortHandle = u64;
class SwapChainBuffers;

enum class GD_COMMON_API PresentPortType
{
	MainPort,
	DebugPort
};

struct GD_COMMON_API WindowInfo
{
	PortHandle		mWindow = {};
	Vec2i			mSize = {};
};

struct GD_COMMON_API PresentPort
{
	PortHandle			mWindow = {};
	Vec2i				mSize = {};
	SwapChainBuffers*	mSwapChain = nullptr;
};
