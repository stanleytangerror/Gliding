#pragma once

class GraphicsContext;
class IRenderTargetView;
class IShaderResourceView;
class D3D12SamplerView;

namespace RenderUtils
{
	void CopyTexture(GraphicsContext* context, 
		IRenderTargetView* target, const Vec2f& targetOffset, const Vec2f& targetRect,
		IShaderResourceView* source, D3D12SamplerView* sourceSampler, const char* sourcePixelUnary = nullptr);

	void GaussianBlur(GraphicsContext* context,
		IRenderTargetView* target, 
		IShaderResourceView* source, i32 kernelSizeInPixel);

	enum WorldStencilMask : u8
	{
		WorldStencilMask_Scene = 0x7f,
		WorldStencilMask_OpaqueObject = 0x1 << 0,
		WorldStencilMask_Sky = 0x1 << 6
	};
}