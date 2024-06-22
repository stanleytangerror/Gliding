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
	FrameGraphMutableResource		mMainViewDepth;
};

class GD_RENDER_API WorldRenderer
{
public:
	WorldRenderer(RenderModule* renderModule, const Vec2u& renderSize);
	virtual ~WorldRenderer();

	void TickFrame(Timer* timer);
	void Render(GI::IGraphicsInfra* infra, FrameGraphMutableResource& target);

	void RenderGBufferChannels(GI::IGraphicsInfra* infra, FrameGraphMutableResource& target);
	void RenderShadowMaskChannel(GI::IGraphicsInfra* infra, FrameGraphMutableResource& target);
	void RenderLightViewDepthChannel(GI::IGraphicsInfra* infra, FrameGraphMutableResource& target);

private:
	void RenderSky(FrameGraph* frameGraph, FrameGraphMutableResource& target, FrameGraphMutableResource& depth) const;
	void DeferredLighting(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, FrameGraphMutableResource& target);

	static void RenderGeometryWithMaterial(FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		std::array<FrameGraphMutableResource, 3>& gbufferRtvs, FrameGraphMutableResource& depthView);

	static void RenderGeometryDepthWithMaterial(FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
		Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		FrameGraphMutableResource& depthView);

	static void RenderShadowMask(FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
		FrameGraphMutableResource& shadowMask,
		FrameGraphResource lightViewDepth, const GI::SamplerDesc& lightViewDepthSampler,
		FrameGraphResource cameraViewDepth, const GI::SamplerDesc& cameraViewDepthSampler);

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

	GI::SamplerDesc mBRDFIntegrationMapSampler;
	GI::SamplerDesc mFilteredEnvMapSampler;

	std::array<std::unique_ptr<GI::IGraphicMemoryResource>, 3> mGBuffers = {};
	std::array<SrvUsageFuture, 3> mGBufferSrvs = {};
	std::array<FrameGraphMutableResource, 3> mGBufferRtvs = {};

	std::unique_ptr<RenderTarget> mShadowMask;

public:

	std::unique_ptr<
		TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>
		>							mTestModel;
};
