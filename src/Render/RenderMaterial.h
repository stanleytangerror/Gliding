#pragma once

#include "World/Scene.h"
#include "Common/GraphicsInfrastructure.h"

struct RenderMaterial
{
	struct MaterialAttriSlot
	{
		class FileTexture* mTexture = nullptr;
		GI::SamplerDesc mSampler;
		Vec4f mConstantValue = Vec4f::Zero();
	};

	std::array<MaterialAttriSlot, TextureUsage_Count> mMatAttriSlots;

	void UpdateGpuResources(GI::IGraphicsInfra* infra);
	bool IsGpuResourceReady() const;

	static RenderMaterial* GenerateRenderMaterialFromRawData(
		const MaterialRawData* matRawData,
		const SceneRawData* sceneRawData,
		const std::map<std::string, class FileTexture*>& textures,
		const std::map<TextureSamplerType, GI::SamplerDesc>& samplers);
};