#pragma once

class RenderModule;
class GraphicsContext;
namespace D3D12Backend
{
	class RenderTargetView;
	class ShaderResourceView;
	class UnorderedAccessView;
}
class D3D12Geometry;

class GD_RENDER_API ScreenRenderer
{
public:
	ScreenRenderer(RenderModule* renderModule);
	virtual ~ScreenRenderer();

	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context, D3D12Backend::ShaderResourceView* sceneHdr, D3D12Backend::RenderTargetView* screenRt);

private:
	void CalcSceneExposure(GraphicsContext* context, D3D12Backend::ShaderResourceView* input, D3D12Backend::UnorderedAccessView* exposureTex);
	void ToneMapping(GraphicsContext* context, D3D12Backend::ShaderResourceView* sceneHdr, D3D12Backend::ShaderResourceView* exposure, D3D12Backend::RenderTargetView* target);

private:
	RenderModule* mRenderModule = nullptr;

	D3D12Geometry* mQuad = nullptr;

	f32 mSecondsSinceLaunch = 0.f;
	f32 mLastFrameDeltaTimeInSeconds = 0.f;

	f32 mEyeAdaptSpeedUp = 3.f;
	f32 mEyeAdaptSpeedDown = 1.f;
};