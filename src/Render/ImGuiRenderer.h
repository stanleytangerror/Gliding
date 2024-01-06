#pragma once

#include "Common/GraphicsInfrastructure.h"

class Timer;
class RenderModule;
struct ImDrawData;

class ImGuiRenderer
{
public:
	ImGuiRenderer(RenderModule* renderModule);

	void TickFrame(Timer* timer);
	void Render(GI::IGraphicsInfra* infra, const GI::RtvDesc& target, ImDrawData* uiData);

protected:
	RenderModule*		mRenderModule = nullptr;
	GI::SamplerDesc		mImGuiSampler;

	GI::SrvDesc			mFontAtlasSrvDesc;
	InMemoryTexture*	mFontAtlas = nullptr;
};