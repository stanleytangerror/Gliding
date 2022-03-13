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
	D3D12Geometry* mSphere = nullptr;
	D3D12Texture* mTex = nullptr;
	std::array<D3D12RenderTarget*, 3> mGBufferRts = {};

	Math::PerspectiveProjection	mCameraProj;
	Vec3f mPosition = { 0.f, 10.f, 0.f };

	Vec3f mDir = { 0.f, 1.f, 0.f };
	Vec3f mUp = { 0.f, 0.f, 1.f };
	Vec3f mRight = { 1.f, 0.f, 0.f };
};
