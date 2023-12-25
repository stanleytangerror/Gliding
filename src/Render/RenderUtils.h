#pragma once

#include "Common/TransformHierarchy.h"
#include "Common/GraphicsInfrastructure.h"
#include "Geometry.h"
#include "RenderMaterial.h"

class GraphicsContext;
namespace D3D12Backend
{
	class RenderTargetView;
	class ShaderResourceView;
	class SamplerView;
}

struct SceneRawData;

namespace RenderUtils
{
	void CopyTexture(GI::IGraphicsInfra* infra,
		const GI::RtvDesc& target, const Vec2f& targetOffset, const Vec2f& targetRect,
		const GI::SrvDesc& source, const GI::SamplerDesc& sourceSampler, const char* sourcePixelUnary = nullptr);

	void CopyTexture(D3D12Backend::GraphicsContext* context,
		D3D12Backend::RenderTargetView* target, 
		D3D12Backend::ShaderResourceView* source, D3D12Backend::SamplerView* sourceSampler);

	void GaussianBlur(D3D12Backend::GraphicsContext* context,
		D3D12Backend::RenderTargetView* target, 
		D3D12Backend::ShaderResourceView* source, i32 kernelSizeInPixel);

	enum WorldStencilMask : u8
	{
		WorldStencilMask_Scene = 0x7f,
		WorldStencilMask_OpaqueObject = 0x1 << 0,
		WorldStencilMask_Sky = 0x1 << 6
	};

	//////////////////////////////////////////////////////////////////////////

	TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>*
	FromSceneRawData(GI::IGraphicInfra* infra, SceneRawData* sceneRawData);

	TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>*
		GenerateMaterialProbes(D3D12Backend::D3D12Device* device);

	Geometry* GenerateGeometryFromMeshRawData(D3D12Backend::D3D12Device* device, const MeshRawData* meshRawData);
}