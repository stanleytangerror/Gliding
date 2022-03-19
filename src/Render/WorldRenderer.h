#pragma once

#include "Common/TransformHierarchy.h"

class RenderModule;
class D3D12Geometry;
class IRenderTargetView;
class GraphicsContext;
class D3D12Texture;
class D3D12RenderTarget;
class D3DDepthStencil;

class GD_RENDER_API WorldRenderer
{
public:
	WorldRenderer(RenderModule* renderModule);

	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context, IRenderTargetView* target);

private:
	void RenderGeometry(GraphicsContext* context, D3D12Geometry* geometry, const Transformf& transform) const;

private:
	RenderModule* mRenderModule = nullptr;

	f32 mElapsedTime = 0.f;

	D3D12Geometry* mQuad = nullptr;
	D3D12Geometry* mSphere = nullptr;
	D3D12Texture* mPanoramicSkyTex = nullptr;

	D3DDepthStencil* mDepthRt = nullptr;
	std::array<D3D12RenderTarget*, 3> mGBufferRts = {};

	Math::PerspectiveProjection	mCameraProj;
	
	Transformf mObjTrans = Transformf::Identity();

	Vec3f mCamPos = { 0.f, -10.f, 0.f };
	Vec3f mDir = { 0.f, 1.f, 0.f };
	Vec3f mUp = { 0.f, 0.f, 1.f };
	Vec3f mRight = { 1.f, 0.f, 0.f };

	TransformNode<D3D12Geometry*> mGismo;
};
