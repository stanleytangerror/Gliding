#pragma once

#include "Common/TransformHierarchy.h"
#include "D3D12Backend/D3D12Geometry.h"
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
	void CopyTexture(GraphicsContext* context, 
		D3D12Backend::RenderTargetView* target, const Vec2f& targetOffset, const Vec2f& targetRect,
		D3D12Backend::ShaderResourceView* source, D3D12Backend::SamplerView* sourceSampler, const char* sourcePixelUnary = nullptr);

	void CopyTexture(GraphicsContext* context,
		D3D12Backend::RenderTargetView* target, 
		D3D12Backend::ShaderResourceView* source, D3D12Backend::SamplerView* sourceSampler);

	void GaussianBlur(GraphicsContext* context,
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
		std::unique_ptr<D3D12Geometry>,
		std::shared_ptr<RenderMaterial>>>*
	FromSceneRawData(D3D12Device* device, SceneRawData* sceneRawData);

	TransformNode<std::pair<
		std::unique_ptr<D3D12Geometry>,
		std::shared_ptr<RenderMaterial>>>*
		GenerateMaterialProbes(D3D12Device* device);

	D3D12Geometry* GenerateGeometryFromMeshRawData(D3D12Device* device, const MeshRawData* meshRawData);
}