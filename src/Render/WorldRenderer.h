#pragma once

#include "Common/TransformHierarchy.h"

class RenderModule;
class D3D12Geometry;
class IRenderTargetView;
class D3D12SamplerView;
class GraphicsContext;
class D3D12Texture;
class D3D12RenderTarget;
class D3DDepthStencil;
class DSV;
struct RenderMaterial;

class GD_RENDER_API WorldRenderer
{
public:
	WorldRenderer(RenderModule* renderModule);
	virtual ~WorldRenderer();

	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context, IRenderTargetView* target);

	void RenderGBufferChannels(GraphicsContext* context, IRenderTargetView* target);

private:
	void RenderSky(GraphicsContext* context, IRenderTargetView* target, DSV* depth) const;
	void RenderGeometryWithMaterial(GraphicsContext* context, D3D12Geometry* geometry, RenderMaterial* material, const Transformf& transform) const;
	void DeferredLighting(GraphicsContext* context, IRenderTargetView* target);

private:
	RenderModule* mRenderModule = nullptr;

	D3D12Geometry* mQuad = nullptr;
	D3D12Geometry* mSphere = nullptr;
	D3D12Texture* mPanoramicSkyTex = nullptr;
	D3D12SamplerView* mPanoramicSkySampler = nullptr;
	D3D12SamplerView* mLightingSceneSampler = nullptr;
	D3D12SamplerView* mNoMipMapLinearSampler = nullptr;

	D3DDepthStencil* mDepthRt = nullptr;
	std::array<D3D12RenderTarget*, 3> mGBufferRts = {};

	Math::PerspectiveProjection	mCameraProj;
	Math::CameraTransformf mCameraTrans;

	TransformNode<std::pair<
		std::unique_ptr<D3D12Geometry>,
		std::shared_ptr<RenderMaterial>>> mTestModel;

	struct DirectionalLight
	{
		Vec3f mLightColor = Vec3f::Zero();
		Vec3f mLightDir = { 0.f, 0.f, 1.f };
	}	mLight;

	const u8	mSceneMask = 0x7f;
	const u8	mOpaqueObjMask = 0x1 << 0;
	const u8	mSkyMask = 0x1 << 6;
};
