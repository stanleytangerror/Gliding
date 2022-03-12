#pragma once

class RenderModule;
class D3D12Geometry;
class IRenderTargetView;
class GraphicsContext;
class D3D12Texture;
class D3D12RenderTarget;

class GD_RENDER_API WorldRenderer
{
public:
	WorldRenderer(RenderModule* renderModule);

	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context, IRenderTargetView* target);

private:
	RenderModule* mRenderModule = nullptr;

	f32	mElapsedTime = 0.f;

	D3D12Geometry* mQuad = nullptr;
	D3D12Texture* mTex = nullptr;
	D3D12RenderTarget* mRt = nullptr;
};
