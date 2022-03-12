#pragma once

class RenderModule;
class GraphicsContext;
class IRenderTargetView;
class IShaderResourceView;
class D3D12Geometry;

class GD_RENDER_API ScreenRenderer
{
public:
	ScreenRenderer(RenderModule* renderModule);

	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context, IShaderResourceView* input, IRenderTargetView* target);

private:
	RenderModule* mRenderModule = nullptr;

	D3D12Geometry* mQuad = nullptr;
};