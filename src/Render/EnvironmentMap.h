#pragma once

class D3D12RenderTarget;
class GraphicsContext;
class IShaderResourceView;
class IRenderTargetView;

class EnvironmentMap
{
public:
	static D3D12RenderTarget* GenerateIrradianceMap(GraphicsContext* context, IShaderResourceView* sky, i32 resolution, i32 semiSphereBusbarSampleCount);
	static D3D12RenderTarget* GenerateIntegratedBRDF(GraphicsContext* context, i32 resolution);
	static D3D12RenderTarget* GeneratePrefilteredEnvironmentMap(GraphicsContext* context, IShaderResourceView* src, i32 resolution);

protected:
	static void PrefilterEnvironmentMap(GraphicsContext* context, IRenderTargetView* target, IShaderResourceView* src, const Vec2i& targetSize, f32 roughness);
};