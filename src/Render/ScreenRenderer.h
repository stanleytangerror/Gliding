#pragma once

class RenderModule;
class RenderResourceManager;
class Geometry;

namespace D3D12Backend
{
	class GraphicsContext;
	class RenderTargetView;
	class ShaderResourceView;
	class UnorderedAccessView;
}

class GD_RENDER_API ScreenRenderer
{
public:
	ScreenRenderer(RenderModule* renderModule);
	virtual ~ScreenRenderer();

	void TickFrame(Timer* timer);
	void Render(D3D12Backend::GraphicsContext* context, D3D12Backend::ShaderResourceView* sceneHdr, D3D12Backend::RenderTargetView* screenRt);

private:
	void CalcSceneExposure(D3D12Backend::GraphicsContext* context, D3D12Backend::ShaderResourceView* input, D3D12Backend::UnorderedAccessView* exposureTex);
	void ToneMapping(D3D12Backend::GraphicsContext* context, D3D12Backend::ShaderResourceView* sceneHdr, D3D12Backend::ShaderResourceView* exposure, D3D12Backend::RenderTargetView* target);

	void TestPass(D3D12Backend::GraphicsContext* context, RenderResourceManager* resMgr);

private:
	RenderModule* mRenderModule = nullptr;

	Geometry* mQuad = nullptr;

	f32 mSecondsSinceLaunch = 0.f;
	f32 mLastFrameDeltaTimeInSeconds = 0.f;

	f32 mEyeAdaptSpeedUp = 3.f;
	f32 mEyeAdaptSpeedDown = 1.f;
};