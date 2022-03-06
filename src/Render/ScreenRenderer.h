#pragma once

#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12CommandContext.h"
#include "D3D12/D3D12Geometry.h"
#include "RenderModule.h"

class RenderModule;
class GraphicsContext;

class GD_RENDER_API ScreenRenderer
{
public:
	ScreenRenderer(RenderModule* renderModule);

	void Initial();
	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context);

private:
	RenderModule* mRenderModule = nullptr;

	f32	mElapsedTime = 0.f;

	D3D12Geometry* mQuad = nullptr;
};