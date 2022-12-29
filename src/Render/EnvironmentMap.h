#pragma once

class D3D12RenderTarget;
class GraphicsContext;
namespace D3D12Backend
{
	class ShaderResourceView;
	class RenderTargetView;
}

namespace D3D12Backend
{
	class CommitedResource;
}

class EnvironmentMap
{
public:
	static std::tuple<D3D12Backend::CommitedResource*, D3D12Backend::ShaderResourceView*> GenerateIrradianceMap(GraphicsContext* context, D3D12Backend::ShaderResourceView* sky, i32 resolution, i32 semiSphereBusbarSampleCount);
	static std::tuple<D3D12Backend::CommitedResource*, D3D12Backend::ShaderResourceView*> GenerateIntegratedBRDF(GraphicsContext* context, i32 resolution);
	static std::tuple<D3D12Backend::CommitedResource*, D3D12Backend::ShaderResourceView*> GeneratePrefilteredEnvironmentMap(GraphicsContext* context, D3D12Backend::ShaderResourceView* src, i32 resolution);

protected:
	static void PrefilterEnvironmentMap(GraphicsContext* context, D3D12Backend::RenderTargetView* target, D3D12Backend::ShaderResourceView* src, const Vec2i& targetSize, f32 roughness);
};