#pragma once

class RenderModule;
class RenderPassManager;
class RenderResourceManager;
class GeometryData;

class GD_RENDER_API TestRenderer
{
public:
	TestRenderer(RenderModule* renderModule);

	void TestRender(RenderPassManager* passMgr, RenderResourceManager* resMgr);

private:
	RenderModule* const mRenderModule = nullptr;
	GeometryData* mQuadGeoData = nullptr;
	std::vector<b8> mTextureData;
};