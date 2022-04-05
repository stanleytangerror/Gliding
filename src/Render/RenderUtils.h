#pragma once

class GraphicsContext;
class IRenderTargetView;
class IShaderResourceView;
class D3D12SamplerView;

namespace RenderUtils
{
	void CopyTexture(GraphicsContext* context, 
		IRenderTargetView* target, const Vec2f& targetOffset, const Vec2f& targetRect,
		IShaderResourceView* source, D3D12SamplerView* sourceSampler);
}