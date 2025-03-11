#pragma once

#include "FrameGraph.h"
#include "Geometry.h"
#include "RenderMaterial.h"
#include "Texture.h"
#include "Common/TransformHierarchy.h"

struct GD_RENDER_API MeshInstance
{
public:
	Guid mGeometryId;
	Guid mRenderMaterialId;
};

class GD_RENDER_API Scene
{
public:
	void AddModelData(FrameGraph* frameGraph, const ModelProcess::Model& model, const Transformf& transform);

	void UpdateAbsoluteTransforms();

	void ForEachMeshInstance(std::function<void(const Transformf&, Geometry*, RenderMaterial*)> func);

	TransformNode<MeshInstance>* GetContentHierarchy() { return mContentHierarchy.get(); }

protected:
	std::unique_ptr<TransformNode<MeshInstance>> mContentHierarchy = std::make_unique<TransformNode<MeshInstance>>();

	std::map<Guid, std::unique_ptr<Geometry>> mGeometries;
	std::map<Guid, std::shared_ptr<RenderMaterial>> mRenderMaterials;
	std::map<Guid, std::pair<std::shared_ptr<FileTexture>, GI::SamplerDesc>> mTextures;
};

