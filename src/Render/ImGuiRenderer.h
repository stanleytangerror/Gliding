#pragma once

#include "Common/GraphicsInfrastructure.h"

class Timer;
class RenderModule;
namespace D3D12Backend
{
	class SamplerView;
	class RenderTargetView;
	class GraphicsContext;
}
struct ImDrawData;
class Texture;

class ImGuiRenderer
{
public:
	ImGuiRenderer(RenderModule* renderModule);

	void TickFrame(Timer* timer);
	void Render(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target, ImDrawData* uiData);

protected:
	RenderModule*		mRenderModule = nullptr;
	D3D12Backend::SamplerView*	mImGuiSampler = nullptr;

	InMemoryTexture*		mFontAtlas = nullptr;
};