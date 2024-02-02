#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "Texture.h"

class Timer;
class RenderModule;
struct ImDrawData;

class ImGuiRenderer
{
public:
	ImGuiRenderer(RenderModule* renderModule);

	void TickFrame(Timer* timer);
	void Render(GI::IGraphicsInfra* infra, const GI::RtvUsage& target, ImDrawData* uiData);

protected:
	RenderModule*		mRenderModule = nullptr;
	GI::SamplerDesc		mImGuiSampler;

	GI::SrvUsage			mFontAtlasSrvDesc;
	std::unique_ptr<InMemoryTexture>	mFontAtlas;
};