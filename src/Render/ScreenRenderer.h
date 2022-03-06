#pragma once

#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12CommandContext.h"
#include "RenderModule.h"

class RenderModule;
class GraphicsContext;

class ScreenRenderer
{
public:
	ScreenRenderer(RenderModule* renderModule);

	void Initial();
	void TickFrame(Timer* timer);
	void Render(GraphicsContext* context);

private:
	RenderModule* mRenderModule = nullptr;

	f32	mElapsedTime = 0.f;

	ID3D12Resource* mVb = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbv = {};

	ID3D12Resource* mIb = nullptr;
	D3D12_INDEX_BUFFER_VIEW mIbv = {};
	
	std::vector < D3D12_INPUT_ELEMENT_DESC > mInputDescs;
};