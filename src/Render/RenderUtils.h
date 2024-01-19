#pragma once

#include "Common/TransformHierarchy.h"
#include "Common/GraphicsInfrastructure.h"
#include "Geometry.h"
#include "RenderMaterial.h"

struct SceneRawData;

namespace RenderUtils
{
	void CopyTexture(GI::IGraphicsInfra* infra,
		const GI::RtvDesc& target, const Vec2f& targetOffset, const Vec2f& targetRect,
		const GI::SrvDesc& source, const GI::SamplerDesc& sourceSampler, const char* sourcePixelUnary = nullptr);

	void CopyTexture(GI::IGraphicsInfra* infra,
		const GI::RtvDesc& target, 
		const GI::SrvDesc& source, const GI::SamplerDesc& sourceSampler);

	void GaussianBlur(GI::IGraphicsInfra* infra,
		const GI::RtvDesc& target, 
		const GI::SrvDesc& source, i32 kernelSizeInPixel);

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
	FromSceneRawData(GI::IGraphicsInfra* infra, SceneRawData* sceneRawData);

	TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>*
		GenerateMaterialProbes(GI::IGraphicsInfra* infra);

	Geometry* GenerateGeometryFromMeshRawData(const MeshRawData* meshRawData);
}