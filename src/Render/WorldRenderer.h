#pragma once

#include "Common/TransformHierarchy.h"

class RenderModule;
class D3D12Geometry;
class IRenderTargetView;
class IShaderResourceView;
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
	WorldRenderer(RenderModule* renderModule, const Vec2i& renderSize);
	virtual ~WorldRenderer();

	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context, IRenderTargetView* target);

	void RenderGBufferChannels(GraphicsContext* context, IRenderTargetView* target);
	void RenderShadowMaskChannel(GraphicsContext* context, IRenderTargetView* target);
	void RenderLightViewDepthChannel(GraphicsContext* context, IRenderTargetView* target);

private:
	void RenderSky(GraphicsContext* context, IRenderTargetView* target, DSV* depth) const;
	void DeferredLighting(GraphicsContext* context, IRenderTargetView* target);

	static void RenderGeometryWithMaterial(GraphicsContext* context, 
		D3D12Geometry* geometry, RenderMaterial* material, 
		const Transformf& transform, 
		const Math::CameraTransformf& cameraTrans, const Math::PerspectiveProjectionf& cameraProj,
		const std::array<D3D12RenderTarget*, 3>& gbufferRts, DSV* depthView);

	static void RenderGeometryDepthWithMaterial(GraphicsContext* context,
		D3D12Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const Math::CameraTransformf& cameraTrans, const Math::OrthographicProjectionf& cameraProj,
		DSV* depthView);

	static void RenderShadowMask(GraphicsContext* context,
		IRenderTargetView* shadowMask,
		IShaderResourceView* lightViewDepth, D3D12SamplerView* lightViewDepthSampler,
		IShaderResourceView* cameraViewDepth, D3D12SamplerView* cameraViewDepthSampler,
		const Math::OrthographicProjectionf& lightViewProj, const Math::CameraTransformf& lightViewTrans,
		const Math::PerspectiveProjectionf& cameraProj, const Math::CameraTransformf& cameraTrans);

private:
	RenderModule* mRenderModule = nullptr;
	Vec2i const mRenderSize = {};

	D3D12Geometry* mQuad = nullptr;
	D3D12Geometry* mSphere = nullptr;

	D3D12Texture* mSkyTexture = nullptr;
	D3D12RenderTarget* mPanoramicSkyRt = nullptr;
	D3D12SamplerView* mPanoramicSkySampler = nullptr;
	f32	mSkyLightIntensity = 50.f;

	D3D12SamplerView* mLightingSceneSampler = nullptr;
	D3D12SamplerView* mNoMipMapLinearSampler = nullptr;
	D3D12SamplerView* mNoMipMapLinearDepthCmpSampler = nullptr;

	D3D12RenderTarget* mBRDFIntegrationMap = nullptr;
	D3D12SamplerView* mBRDFIntegrationMapSampler = nullptr;

	D3D12RenderTarget* mIrradianceMap = nullptr;
	D3D12RenderTarget* mFilteredEnvMap = nullptr;
	D3D12SamplerView* mFilteredEnvMapSampler = nullptr;

	DirectionalLight* mSunLight = nullptr;
	D3DDepthStencil* mLightViewDepthRt = nullptr;

	D3DDepthStencil* mMainDepthRt = nullptr;
	std::array<D3D12RenderTarget*, 3> mGBufferRts = {};
	D3D12RenderTarget* mShadowMask = nullptr;

public:
	Math::PerspectiveProjectionf	mCameraProj;
	Math::CameraTransformf			mCameraTrans;

	TransformNode<std::pair<
		std::unique_ptr<D3D12Geometry>,
		std::shared_ptr<RenderMaterial>>>* mTestModel;
};
