#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "Texture.h"
#include "FrameGraph.h"

class Timer;
class RenderModule;
struct ImDrawData;

class ImGuiRenderer
{
public:
	ImGuiRenderer(RenderModule* renderModule);

	void TickFrame(Timer* timer);
	void Render(GI::IGraphicsInfra* infra, FrameGraphMutableResource& target, ImDrawData* uiData);

protected:
	RenderModule*		mRenderModule = nullptr;
	GI::SamplerDesc		mImGuiSampler;

	std::unique_ptr<InMemoryTexture>	mFontAtlas;
};