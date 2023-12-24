#pragma once

#include "Common/TransformHierarchy.h"
#include "Common/GraphicsInfrastructure.h"

class RenderModule;
class Geometry;
class Texture;
class RenderTarget;
namespace D3D12Backend
{
	class GraphicsContext;
	class CommitedResource;
	class SamplerView;
	class ShaderResourceView;
	class RenderTargetView;
	class UnorderedAccessView;
	class DepthStencilView;
}
struct RenderMaterial;
struct DirectionalLight;

class GD_RENDER_API WorldRenderer
{
public:
	WorldRenderer(RenderModule* renderModule, const Vec2i& renderSize);
	virtual ~WorldRenderer();

	void TickFrame(Timer* timer);
	void Render(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target);

	void RenderGBufferChannels(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target);
	void RenderShadowMaskChannel(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target);
	void RenderLightViewDepthChannel(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target);

private:
	void RenderSky(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target, D3D12Backend::DepthStencilView* depth) const;
	void DeferredLighting(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target);

	static void RenderGeometryWithMaterial(D3D12Backend::GraphicsContext* context,
		Geometry* geometry, RenderMaterial* material, 
		const Transformf& transform, 
		const Math::CameraTransformf& cameraTrans, const Math::PerspectiveProjectionf& cameraProj,
		const std::array<D3D12Backend::RenderTargetView*, 3>& gbufferRts, D3D12Backend::DepthStencilView* depthView);

	static void RenderGeometryWithMaterialNew(D3D12Backend::GraphicsContext* context,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const Math::CameraTransformf& cameraTrans, const Math::PerspectiveProjectionf& cameraProj,
		const std::array<GI::RtvDesc, 3>& gbufferRtvs, const GI::DsvDesc& depthView,
		const Vec2i& targetSize);

	static void RenderGeometryDepthWithMaterial(D3D12Backend::GraphicsContext* context,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const Math::CameraTransformf& cameraTrans, const Math::OrthographicProjectionf& cameraProj,
		D3D12Backend::DepthStencilView* depthView);

	static void RenderShadowMask(D3D12Backend::GraphicsContext* context,
		D3D12Backend::RenderTargetView* shadowMask,
		D3D12Backend::ShaderResourceView* lightViewDepth, D3D12Backend::SamplerView* lightViewDepthSampler,
		D3D12Backend::ShaderResourceView* cameraViewDepth, D3D12Backend::SamplerView* cameraViewDepthSampler,
		const Math::OrthographicProjectionf& lightViewProj, const Math::CameraTransformf& lightViewTrans,
		const Math::PerspectiveProjectionf& cameraProj, const Math::CameraTransformf& cameraTrans);

private:
	RenderModule* mRenderModule = nullptr;
	Vec2i const mRenderSize = {};

	Geometry* mQuad = nullptr;
	Geometry* mSphere = nullptr;

	Texture* mSkyTexture = nullptr;
	RenderTarget* mPanoramicSkyRt = nullptr;
	D3D12Backend::SamplerView* mPanoramicSkySampler = nullptr;
	f32	mSkyLightIntensity = 50.f;

	D3D12Backend::SamplerView* mLightingSceneSampler = nullptr;
	D3D12Backend::SamplerView* mNoMipMapLinearSampler = nullptr;
	D3D12Backend::SamplerView* mNoMipMapLinearDepthCmpSampler = nullptr;

	D3D12Backend::CommitedResource* mBRDFIntegrationMap = nullptr;
	D3D12Backend::ShaderResourceView* mBRDFIntegrationMapSrv = nullptr;
	D3D12Backend::SamplerView* mBRDFIntegrationMapSampler = nullptr;

	D3D12Backend::CommitedResource* mIrradianceMap = nullptr;
	D3D12Backend::ShaderResourceView* mIrradianceMapSrv = nullptr;

	D3D12Backend::CommitedResource* mFilteredEnvMap = nullptr;
	D3D12Backend::ShaderResourceView* mFilteredEnvMapSrv = nullptr;
	D3D12Backend::SamplerView* mFilteredEnvMapSampler = nullptr;

	DirectionalLight* mSunLight = nullptr;

	D3D12Backend::CommitedResource* mLightViewDepth = nullptr;
	D3D12Backend::DepthStencilView* mLightViewDepthDsv = nullptr;
	D3D12Backend::ShaderResourceView* mLightViewDepthSrv = nullptr;

	D3D12Backend::CommitedResource* mMainDepth = nullptr;
	D3D12Backend::DepthStencilView* mMainDepthDsv = nullptr;
	D3D12Backend::ShaderResourceView* mMainDepthSrv = nullptr;

	std::array<D3D12Backend::CommitedResource*, 3> mGBuffers = {};
	std::array<D3D12Backend::ShaderResourceView*, 3> mGBufferSrvs = {};
	std::array<D3D12Backend::RenderTargetView*, 3> mGBufferRtvs = {};

	RenderTarget* mShadowMask = nullptr;

public:
	Math::PerspectiveProjectionf	mCameraProj;
	Math::CameraTransformf			mCameraTrans;

	TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>* mTestModel;
};
