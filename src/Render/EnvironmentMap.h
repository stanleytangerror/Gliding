#pragma once

class D3D12RenderTarget;
namespace D3D12Backend
{
	class GraphicsContext;
	class ShaderResourceView;
	class RenderTargetView;
	class CommitedResource;
}

class EnvironmentMap
{
public:
	static std::tuple<D3D12Backend::CommitedResource*, D3D12Backend::ShaderResourceView*> GenerateIrradianceMap(D3D12Backend::GraphicsContext* context, D3D12Backend::ShaderResourceView* sky, i32 resolution, i32 semiSphereBusbarSampleCount);
	static std::tuple<D3D12Backend::CommitedResource*, D3D12Backend::ShaderResourceView*> GenerateIntegratedBRDF(D3D12Backend::GraphicsContext* context, i32 resolution);
	static std::tuple<D3D12Backend::CommitedResource*, D3D12Backend::ShaderResourceView*> GeneratePrefilteredEnvironmentMap(D3D12Backend::GraphicsContext* context, D3D12Backend::ShaderResourceView* src, i32 resolution);

protected:
	static void PrefilterEnvironmentMap(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target, D3D12Backend::ShaderResourceView* src, const Vec2i& targetSize, f32 roughness);
};