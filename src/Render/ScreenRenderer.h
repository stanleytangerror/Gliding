#pragma once

#include "D3D12/D3D12Headers.h"
#include "RenderModule.h"

class RenderModule;

class ScreenRenderer
{
public:
	ScreenRenderer(RenderModule* renderModule);

	void Initial();
	void Render();

private:
	RenderModule* mRenderModule = nullptr;

	ID3D12Resource* mVb = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbv = {};

	ID3D12Resource* mIb = nullptr;
	D3D12_INDEX_BUFFER_VIEW mIbv = {};
	
	std::vector < D3D12_INPUT_ELEMENT_DESC > mInputDescs;
};