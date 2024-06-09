#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "Common/TransformHierarchy.h"
#include "RenderTarget.h"
#include "Texture.h"
#include "Geometry.h"
#include "RenderMaterial.h"
#include "Light.h"

class RenderModule;

class GD_RENDER_API WorldRenderer
{
public:
	WorldRenderer(RenderModule* renderModule, const Vec2u& renderSize);
	virtual ~WorldRenderer();

	void TickFrame(Timer* timer);
	void Render(GI::IGraphicsInfra* infra, const GI::RtvUsage& target);

	void RenderGBufferChannels(GI::IGraphicsInfra* infra, const GI::RtvUsage& target);
	void RenderShadowMaskChannel(GI::IGraphicsInfra* infra, const GI::RtvUsage& target);
	void RenderLightViewDepthChannel(GI::IGraphicsInfra* infra, const GI::RtvUsage& target);

private:
	void RenderSky(GI::IGraphicsInfra* infra, const GI::RtvUsage& target, const GI::DsvUsage& depth) const;
	void DeferredLighting(GI::IGraphicsInfra* infra, const GI::RtvUsage& target);

	static void RenderGeometryWithMaterial(GI::IGraphicsInfra* infra,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const Math::CameraTransformf& cameraTrans, const Math::PerspectiveProjectionf& cameraProj,
		const std::array<GI::RtvUsage, 3>& gbufferRtvs, const GI::DsvUsage& depthView);

	static void RenderGeometryDepthWithMaterial(GI::IGraphicsInfra* infra,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const Math::CameraTransformf& cameraTrans, const Math::OrthographicProjectionf& cameraProj,
		const GI::DsvUsage& depthView);

	static void RenderShadowMask(GI::IGraphicsInfra* infra,
		const GI::RtvUsage& shadowMask,
		const GI::SrvUsage& lightViewDepth, const GI::SamplerDesc& lightViewDepthSampler,
		const GI::SrvUsage& cameraViewDepth, const GI::SamplerDesc& cameraViewDepthSampler,
		const Math::OrthographicProjectionf& lightViewProj, const Math::CameraTransformf& lightViewTrans,
		const Math::PerspectiveProjectionf& cameraProj, const Math::CameraTransformf& cameraTrans);

private:
	RenderModule*	mRenderModule = nullptr;
	Vec2u			mRenderSize = {};

	std::unique_ptr<Geometry> mQuad;
	std::unique_ptr<Geometry> mSphere;

	std::unique_ptr<FileTexture> mSkyTexture;
	std::unique_ptr<RenderTarget> mPanoramicSkyRt;
	GI::SamplerDesc mPanoramicSkySampler;
	f32	mSkyLightIntensity = 50.f;

	GI::SamplerDesc mLightingSceneSampler;
	GI::SamplerDesc mNoMipMapLinearSampler;
	GI::SamplerDesc mNoMipMapLinearDepthCmpSampler;

	std::unique_ptr<GI::IGraphicMemoryResource> mBRDFIntegrationMap;
	GI::SrvUsage mBRDFIntegrationMapSrv;
	GI::SamplerDesc mBRDFIntegrationMapSampler;

	std::unique_ptr<GI::IGraphicMemoryResource> mIrradianceMap;
	GI::SrvUsage mIrradianceMapSrv;

	std::unique_ptr<GI::IGraphicMemoryResource> mFilteredEnvMap;
	GI::SrvUsage mFilteredEnvMapSrv;
	GI::SamplerDesc mFilteredEnvMapSampler;

	DirectionalLight* mSunLight = nullptr;

	std::unique_ptr<GI::IGraphicMemoryResource> mLightViewDepth;
	GI::DsvUsage mLightViewDepthDsv;
	GI::SrvUsage mLightViewDepthSrv;

	std::unique_ptr<GI::IGraphicMemoryResource> mMainDepth;
	GI::DsvUsage mMainDepthDsv;
	GI::SrvUsage mMainDepthSrv;

	std::array<std::unique_ptr<GI::IGraphicMemoryResource>, 3> mGBuffers = {};
	std::array<GI::SrvUsage, 3> mGBufferSrvs = {};
	std::array<GI::RtvUsage, 3> mGBufferRtvs = {};

	std::unique_ptr<RenderTarget> mShadowMask;

public:
	Math::PerspectiveProjectionf	mCameraProj;
	Math::CameraTransformf			mCameraTrans;

	std::unique_ptr<
		TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>
		>							mTestModel;
};
