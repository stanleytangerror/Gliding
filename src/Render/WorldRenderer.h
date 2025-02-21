#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "Common/TransformHierarchy.h"
#include "Scene.h"
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
	FrameGraphMutableResource		mShadowMask;
};

class GD_RENDER_API WorldRenderer
{
public:
	WorldRenderer(RenderModule* renderModule, const Vec2u& renderSize);
	virtual ~WorldRenderer();

	void TickFrame(Timer* timer);
	FrameGraphMutableResource Render();

	void RenderGBufferChannels(FrameGraphMutableResource& target);
	void RenderShadowMaskChannel(FrameGraphMutableResource& target);
	void RenderLightViewDepthChannel(FrameGraphMutableResource& target);

private:
	void RenderSky(FrameGraph* frameGraph, FrameGraphMutableResource& target, FrameGraphMutableResource& depth) const;
	void DeferredLighting(FrameGraph* frameGraph, FrameGraphMutableResource& target);

	static void RenderGeometryWithMaterial(FrameGraph* frameGraph, 
		const Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		std::array<FrameGraphMutableResource, 3>& gbufferRtvs, FrameGraphMutableResource& depthView);

	static void RenderGeometryDepthWithMaterial(FrameGraph* frameGraph, 
		const Geometry* geometry, RenderMaterial* material,
		const Transformf& transform,
		FrameGraphMutableResource& depthView);

	void RenderShadowMask(FrameGraph* frameGraph,
		FrameGraphMutableResource& shadowMask,
		FrameGraphResource lightViewDepth, const GI::SamplerDesc& lightViewDepthSampler,
		FrameGraphResource cameraViewDepth, const GI::SamplerDesc& cameraViewDepthSampler);

private:
	RenderModule*	mRenderModule = nullptr;
	Vec2u			mRenderSize = {};

	std::unique_ptr<GeometryCollection> mGeometryCollection = std::make_unique<GeometryCollection>();

	std::unique_ptr<FileTexture> mSkyTexture;
	GI::SamplerDesc mPanoramicSkySampler;
	f32	mSkyLightIntensity = 50.f;

	GI::SamplerDesc mLightingSceneSampler;
	GI::SamplerDesc mNoMipMapLinearSampler;
	GI::SamplerDesc mNoMipMapLinearDepthCmpSampler;

	GI::SamplerDesc mBRDFIntegrationMapSampler;
	GI::SamplerDesc mFilteredEnvMapSampler;

public:
	std::unique_ptr<Scene>			mScene = std::make_unique<Scene>();
};
