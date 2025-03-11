#pragma once

#include "Common/TransformHierarchy.h"
#include "Common/GraphicsInfrastructure.h"
#include "Geometry.h"
#include "RenderMaterial.h"
#include "FrameGraph.h"

struct SceneRawData;

namespace ModelProcess
{
	struct Mesh;
	struct Model;
	struct Material;
}

namespace RenderUtils
{
	void CopyTexture(FrameGraph* frameGraph, 
		FrameGraphMutableResource& target,
		const Vec2f& targetOffset, const Vec2f& targetRect,
		const Geometry* quad,
		const FrameGraphResource& source,
		const GI::SamplerDesc& sourceSampler, const char* sourcePixelUnary = nullptr);

	void CopyTexture(FrameGraph* frameGraph, 
		FrameGraphMutableResource& target,
		const FrameGraphResource& source,
		const Geometry* quad,
		const GI::SamplerDesc& sourceSampler);

	void GaussianBlur(
		const Geometry* quad,
		FrameGraph* frameGraph,
		FrameGraphMutableResource& target,
		const FrameGraphResource& source, i32 kernelSizeInPixel);

	enum WorldStencilMask : u8
	{
		WorldStencilMask_Scene = 0x7f,
		WorldStencilMask_OpaqueObject = 0x1 << 0,
		WorldStencilMask_Sky = 0x1 << 6
	};

	//////////////////////////////////////////////////////////////////////////

	//TransformNode<std::pair<
	//	std::unique_ptr<Geometry>,
	//	std::shared_ptr<RenderMaterial>>>*
	//	GenerateMaterialProbes(const Geometry* geo, FrameGraph* frameGraph);

	std::unique_ptr<Geometry> GenerateGeometryFromMeshData(const ModelProcess::Mesh& mesh);
}