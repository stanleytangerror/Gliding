#pragma once

#include "Common/TransformHierarchy.h"
#include "D3D12Backend/D3D12Geometry.h"
#include "RenderMaterial.h"

class GraphicsContext;
class IRenderTargetView;
class IShaderResourceView;
class D3D12SamplerView;
struct SceneRawData;

namespace RenderUtils
{
	void CopyTexture(GraphicsContext* context, 
		IRenderTargetView* target, const Vec2f& targetOffset, const Vec2f& targetRect,
		IShaderResourceView* source, D3D12SamplerView* sourceSampler, const char* sourcePixelUnary = nullptr);

	void CopyTexture(GraphicsContext* context,
		IRenderTargetView* target, 
		IShaderResourceView* source, D3D12SamplerView* sourceSampler);

	void GaussianBlur(GraphicsContext* context,
		IRenderTargetView* target, 
		IShaderResourceView* source, i32 kernelSizeInPixel);

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