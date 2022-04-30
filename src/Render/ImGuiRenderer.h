#pragma once

class Timer;
class RenderModule;
class D3D12SamplerView;
class GraphicsContext;
class IRenderTargetView;
struct ImDrawData;
class D3D12Texture;

class ImGuiRenderer
{
public:
	ImGuiRenderer(RenderModule* renderModule);

	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context, IRenderTargetView* target, ImDrawData* uiData);

protected:
	RenderModule*		mRenderModule = nullptr;
	D3D12SamplerView*	mImGuiSampler = nullptr;

	D3D12Texture*		mFontAtlas = nullptr;
};