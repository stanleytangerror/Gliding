#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "Common/TransformHierarchy.h"
#include "RenderTarget.h"
#include "Texture.h"
#include "Geometry.h"
#include "RenderMaterial.h"
#include "Light.h"
#include "FrameGraph.h"

class RenderModule;

struct GD_RENDER_API MainCameraState
{
	Math::PerspectiveProjectionf	mCameraProj;
	Math::CameraTransformf			mCameraTrans;
};

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
	void RenderSky(FrameGraph* frameGraph, const GI::RtvUsage& target, const GI::DsvUsage& depth) const;
	void DeferredLighting(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, const GI::RtvUsage& target);

	static void RenderGeometryWithMaterial(FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const std::array<GI::RtvUsage, 3>& gbufferRtvs, const GI::DsvUsage& depthView);

	static void RenderGeometryDepthWithMaterial(FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		const GI::DsvUsage& depthView);

	static void RenderShadowMask(FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
		const GI::RtvUsage& shadowMask,
		const GI::SrvUsage& lightViewDepth, const GI::SamplerDesc& lightViewDepthSampler,
		const GI::SrvUsage& cameraViewDepth, const GI::SamplerDesc& cameraViewDepthSampler);

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

	std::unique_ptr<
		TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>
		>							mTestModel;
};
