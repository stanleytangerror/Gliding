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
struct DirectionalLight;

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
	void DeferredLighting(GraphicsContext* context, IRenderTargetView* target);

	static void RenderGeometryWithMaterial(GraphicsContext* context, 
		D3D12Geometry* geometry, RenderMaterial* material, 
		const Transformf& transform, 
		const Math::CameraTransformf& cameraTrans, const Math::PerspectiveProjection& cameraProj,
		const std::array<D3D12RenderTarget*, 3>& gbufferRts, DSV* depthView);

	static void RenderGeometryDepthWithMaterial(GraphicsContext* context,
		D3D12Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const Math::CameraTransformf& cameraTrans, const Math::OrthographicProjection& cameraProj,
		DSV* depthView);

private:
	RenderModule* mRenderModule = nullptr;

	D3D12Geometry* mQuad = nullptr;
	D3D12Geometry* mSphere = nullptr;
	D3D12Texture* mPanoramicSkyTex = nullptr;
	D3D12SamplerView* mPanoramicSkySampler = nullptr;
	D3D12SamplerView* mLightingSceneSampler = nullptr;
	D3D12SamplerView* mNoMipMapLinearSampler = nullptr;

	DirectionalLight* mSunLight = nullptr;
	D3DDepthStencil* mLightViewDepthRt = nullptr;

	D3DDepthStencil* mMainDepthRt = nullptr;
	std::array<D3D12RenderTarget*, 3> mGBufferRts = {};

	Math::PerspectiveProjection	mCameraProj;
	Math::CameraTransformf mCameraTrans;

	TransformNode<std::pair<
		std::unique_ptr<D3D12Geometry>,
		std::shared_ptr<RenderMaterial>>> mTestModel;
};
