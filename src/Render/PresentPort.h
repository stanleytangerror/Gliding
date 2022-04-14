#pragma once

class SwapChainBuffers;

enum class GD_RENDER_API PresentPortType
{
	MainPort,
	DebugPort
};

struct GD_RENDER_API WindowInfo
{
	HWND			mWindow = {};
	Vec2i			mSize = {};
};

struct GD_RENDER_API PresentPort
{
	HWND				mWindow = {};
	Vec2i				mSize = {};
	SwapChainBuffers*	mSwapChain = nullptr;
};
