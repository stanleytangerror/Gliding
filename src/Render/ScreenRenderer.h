#pragma once

class RenderModule;
class GraphicsContext;
class IRenderTargetView;
class IShaderResourceView;
class IUnorderedAccessView;
class D3D12Geometry;

class GD_RENDER_API ScreenRenderer
{
public:
	ScreenRenderer(RenderModule* renderModule);
	virtual ~ScreenRenderer();

	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context, IShaderResourceView* sceneHdr, IRenderTargetView* screenRt);

private:
	void CalcSceneExposure(GraphicsContext* context, IShaderResourceView* input, IUnorderedAccessView* exposureTex);
	void ToneMapping(GraphicsContext* context, IShaderResourceView* sceneHdr, IShaderResourceView* exposure, IRenderTargetView* target);

private:
	RenderModule* mRenderModule = nullptr;

	D3D12Geometry* mQuad = nullptr;
};