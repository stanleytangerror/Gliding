#pragma once

#include "Common/TransformHierarchy.h"
#include "Common/GraphicsInfrastructure.h"

class RenderModule;
class Geometry;
class FileTexture;
class RenderTarget;
struct RenderMaterial;
struct DirectionalLight;

class GD_RENDER_API WorldRenderer
{
public:
	WorldRenderer(RenderModule* renderModule, const Vec2i& renderSize);
	virtual ~WorldRenderer();

	void TickFrame(Timer* timer);
	void Render(GI::IGraphicInfra* infra, const GI::RtvDesc& target);

	void RenderGBufferChannels(GI::IGraphicInfra* infra, const GI::RtvDesc& target);
	void RenderShadowMaskChannel(GI::IGraphicInfra* infra, const GI::RtvDesc& target);
	void RenderLightViewDepthChannel(GI::IGraphicInfra* infra, const GI::RtvDesc& target);

private:
	void RenderSky(GI::IGraphicInfra* infra, const GI::RtvDesc& target, const GI::DsvDesc& depth) const;
	void DeferredLighting(GI::IGraphicInfra* infra, const GI::RtvDesc& target);

	static void RenderGeometryWithMaterial(GI::IGraphicInfra* infra,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const Math::CameraTransformf& cameraTrans, const Math::PerspectiveProjectionf& cameraProj,
		const std::array<GI::RtvDesc, 3>& gbufferRtvs, const GI::DsvDesc& depthView,
		const Vec2i& targetSize);

	static void RenderGeometryDepthWithMaterial(GI::IGraphicInfra* infra,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const Math::CameraTransformf& cameraTrans, const Math::OrthographicProjectionf& cameraProj,
		const GI::DsvDesc& depthView,
		const Vec2i& targetSize);

	static void RenderShadowMask(GI::IGraphicInfra* infra,
		const GI::RtvDesc& shadowMask,
		const GI::SrvDesc& lightViewDepth, GI::SamplerDesc lightViewDepthSampler,
		const GI::SrvDesc& cameraViewDepth, GI::SamplerDesc cameraViewDepthSampler,
		const Math::OrthographicProjectionf& lightViewProj, const Math::CameraTransformf& lightViewTrans,
		const Math::PerspectiveProjectionf& cameraProj, const Math::CameraTransformf& cameraTrans);

private:
	RenderModule* mRenderModule = nullptr;
	Vec2i const mRenderSize = {};

	Geometry* mQuad = nullptr;
	Geometry* mSphere = nullptr;

	FileTexture* mSkyTexture = nullptr;
	RenderTarget* mPanoramicSkyRt = nullptr;
	GI::SamplerDesc mPanoramicSkySampler;
	f32	mSkyLightIntensity = 50.f;

	GI::SamplerDesc mLightingSceneSampler;
	GI::SamplerDesc mNoMipMapLinearSampler;
	GI::SamplerDesc mNoMipMapLinearDepthCmpSampler;

	std::unique_ptr<GI::IGraphicMemoryResource> mBRDFIntegrationMap;
	GI::SrvDesc mBRDFIntegrationMapSrv;
	GI::SamplerDesc mBRDFIntegrationMapSampler;

	std::unique_ptr<GI::IGraphicMemoryResource> mIrradianceMap;
	GI::SrvDesc mIrradianceMapSrv;

	std::unique_ptr<GI::IGraphicMemoryResource> mFilteredEnvMap;
	GI::SrvDesc mFilteredEnvMapSrv;
	GI::SamplerDesc mFilteredEnvMapSampler;

	DirectionalLight* mSunLight = nullptr;

	std::unique_ptr<GI::IGraphicMemoryResource> mLightViewDepth;
	GI::DsvDesc mLightViewDepthDsv;
	GI::SrvDesc mLightViewDepthSrv;

	std::unique_ptr<GI::IGraphicMemoryResource> mMainDepth;
	GI::DsvDesc mMainDepthDsv;
	GI::SrvDesc mMainDepthSrv;

	std::array<std::unique_ptr<GI::IGraphicMemoryResource>, 3> mGBuffers = {};
	std::array<GI::SrvDesc, 3> mGBufferSrvs = {};
	std::array<GI::RtvDesc, 3> mGBufferRtvs = {};

	RenderTarget* mShadowMask = nullptr;

public:
	Math::PerspectiveProjectionf	mCameraProj;
	Math::CameraTransformf			mCameraTrans;

	TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>* mTestModel;
};
