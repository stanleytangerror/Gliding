#include "RenderPch.h"
#include "RenderMaterial.h"
#include "Texture.h"

void RenderMaterial::UpdateGpuResources(GI::IGraphicsInfra* infra)
{
	for (const auto& slot : mMatAttriSlots)
	{
		if (slot.mTexture && !slot.mTexture->IsD3DResourceReady())
		{
			slot.mTexture->CreateAndInitialResource(infra);
		}
	}
}

bool RenderMaterial::IsGpuResourceReady() const
{
	for (const auto& slot : mMatAttriSlots)
	{
		if (slot.mTexture && !slot.mTexture->IsD3DResourceReady())
		{
			if (!slot.mTexture->IsD3DResourceReady())
			{
				return false;
			}
		}
	}

	return true;
}

RenderMaterial* RenderMaterial::GenerateRenderMaterialFromRawData(
		const MaterialRawData* matRawData,
		const SceneRawData* sceneRawData,
		const std::map<std::string, FileTexture*>& textures,
		const std::map<TextureSamplerType, GI::SamplerDesc>& samplers)
{
	RenderMaterial* result = new RenderMaterial;

	for (i32 slotIdx = 0; slotIdx < TextureUsage_Count; ++slotIdx)
	{
		const MaterialRawData::ParamBasicInfo& slotInfo = matRawData->mParamSemanticSlots[slotIdx];
		MaterialAttriSlot& attr = result->mMatAttriSlots[slotIdx];
		{
			attr.mConstantValue = slotInfo.mConstantValue;

			auto itt = textures.find(slotInfo.mTexturePath);
			attr.mTexture = (itt != textures.end() ? itt->second : nullptr);

			auto its = samplers.find(slotInfo.mSamplerType);
			attr.mSampler = (its != samplers.end() ? its->second : GI::SamplerDesc());
		}
	}

	return result;
}
