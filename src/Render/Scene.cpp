#include "Render/RenderPch.h"
#include "Scene.h"
#include "Geometry.h"
#include "RenderUtils.h"

void Scene::AddModelData(FrameGraph* frameGraph, const ModelProcess::Model& model, const Transformf& transform)
{
	const auto& modelDirectory = std::filesystem::path(model.Name).parent_path();

	auto result = new TransformNode<std::pair<
		std::shared_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>;

	for (const auto& tex : model.Textures)
	{
		Assert(mTextures.find(tex.mId) == mTextures.end());

		auto texturePath = std::filesystem::path(tex.mPath).is_relative() ?
			(modelDirectory / tex.mPath).string() : tex.mPath;

		const auto& content = Utils::LoadFileContent(texturePath.c_str());

		auto sampler = GI::SamplerDesc()
			.SetFilter(GI::Filter::MIN_MAG_MIP_LINEAR)
			.SetAddress({
				tex.mSampler.mAddressMode[0],
				tex.mSampler.mAddressMode[1],
				tex.mSampler.mAddressMode[2]
				});

		mTextures[tex.mId] = {
			std::make_shared<FileTexture>(frameGraph, texturePath.c_str(), std::span(content)),
			sampler
		};
	}

	for (const auto& mat : model.Materials)
	{
		Assert(mRenderMaterials.find(mat.Id) == mRenderMaterials.end());
		mRenderMaterials[mat.Id] = std::shared_ptr<RenderMaterial>(RenderMaterial::GenerateRenderMaterialFromMaterialData(frameGraph, mat, mTextures));
	}

	std::map<Guid, Guid> geo2Mat;
	for (const auto& mesh : model.Meshes)
	{
		Assert(mGeometries.find(mesh.Id) == mGeometries.end());
		Assert(geo2Mat.find(mesh.Id) == geo2Mat.end());

		auto geo = RenderUtils::GenerateGeometryFromMeshData(mesh);
		geo->CreateAndInitialResource(frameGraph);
		mGeometries[mesh.Id] = std::move(geo);
		geo2Mat[mesh.Id] = mesh.MaterialId;
	}

	TransformNode<MeshInstance>* node = mContentHierarchy->PushChild({}, transform);
	for (const auto& inst : model.MeshInstances)
	{
		node->PushChild(MeshInstance{ inst.MeshId, geo2Mat[inst.MeshId] }, Transformf(inst.LocalTransform));
	}
}

void Scene::UpdateAbsoluteTransforms()
{
	mContentHierarchy->CalcAbsTransform();
}

void Scene::ForEachMeshInstance(std::function<void(const Transformf&, Geometry*, RenderMaterial*)> func)
{
	mContentHierarchy->ForEach([this, func](const TransformNode<MeshInstance>& node)
		{
			const auto& meshInst = node.mContent;
			if (meshInst.mGeometryId.IsValid() && meshInst.mRenderMaterialId.IsValid())
			{
				Assert(this->mGeometries.find(meshInst.mGeometryId) != this->mGeometries.end());
				Assert(this->mRenderMaterials.find(meshInst.mRenderMaterialId) != this->mRenderMaterials.end());

				func(node.mAbsTransform, this->mGeometries[meshInst.mGeometryId].get(), this->mRenderMaterials[meshInst.mRenderMaterialId].get());
			}
		});
}