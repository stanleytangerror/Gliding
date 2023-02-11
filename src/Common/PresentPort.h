#pragma once

#include "Math.h"

using PortHandle = u64;
namespace D3D12Backend
{
	class SwapChainBuffers;
}

enum class GD_COMMON_API PresentPortType
{
	MainPort,
	DebugPort,
	Size
};

struct GD_COMMON_API WindowRuntimeInfo
{
	PortHandle		mNativeHandle = {};
	Vec2i			mSize = {};
};

struct GD_COMMON_API PresentPort
{
	PortHandle			mNativeHandle = {};
	Vec2i				mSize = {};
	D3D12Backend::SwapChainBuffers*	mSwapChain = nullptr;
};
