#pragma once

class D3D12RenderTarget;
class GraphicsContext;
class IShaderResourceView;

class EnvironmentMap
{
public:
	static D3D12RenderTarget* GenerateIrradianceMap(GraphicsContext* context, IShaderResourceView* sky, i32 resolution, i32 semiSphereBusbarSampleCount);
	static D3D12RenderTarget* GenerateIntegratedBRDF(GraphicsContext* context, i32 resolution);
};