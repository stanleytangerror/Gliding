#pragma once

#include "Common/GraphicsInfrastructure.h"

class RenderModule;
class Geometry;

class GD_RENDER_API ScreenRenderer
{
public:
	ScreenRenderer(RenderModule* renderModule);
	virtual ~ScreenRenderer();

	void TickFrame(Timer* timer);
	void Render(GI::IGraphicsInfra* infra, const GI::SrvUsage& sceneHdr, const GI::RtvUsage& screenRt);

private:
	void CalcSceneExposure(GI::IGraphicsInfra* infra, const GI::SrvUsage& input, const GI::UavUsage& exposureTex);
	void ToneMapping(GI::IGraphicsInfra* infra, const GI::SrvUsage& sceneHdr, const GI::SrvUsage& exposure, const GI::RtvUsage& target);

private:
	RenderModule* mRenderModule = nullptr;

	Geometry* mQuad = nullptr;

	f32 mSecondsSinceLaunch = 0.f;
	f32 mLastFrameDeltaTimeInSeconds = 0.f;

	f32 mEyeAdaptSpeedUp = 3.f;
	f32 mEyeAdaptSpeedDown = 1.f;
};