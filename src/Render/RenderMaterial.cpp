#include "RenderPch.h"
#include "RenderMaterial.h"
#include "D3D12/D3D12CommandContext.h"
#include "D3D12/D3D12Texture.h"

void RenderMaterial::UpdateGpuResources(D3D12CommandContext* context)
{
	for (const std::vector<D3D12Texture*>& textures : mTextureParams)
	{
		for (D3D12Texture* const tex : textures)
		{
			if (!tex->IsD3DResourceReady())
			{
				tex->Initial(context);
			}
		}
	}
}

bool RenderMaterial::IsGpuResourceReady() const
{
	for (const std::vector<D3D12Texture*>& textures : mTextureParams)
	{
		for (D3D12Texture* const tex : textures)
		{
			if (!tex->IsD3DResourceReady())
			{
				return false;
			}
		}
	}

	return true;
}

RenderMaterial* RenderMaterial::GenerateRenderMaterialFromRawData(
	const MaterialRawData* rawData, 
	const std::map<std::string, std::pair<D3D12Texture*, D3D12SamplerView*>>& textures)
{
	RenderMaterial* result = new RenderMaterial;

	for (i32 texUsage = 0; texUsage < TextureUsage_Count; ++texUsage)
	{
		const auto& texInfos = rawData->mTexturesWithUsage[texUsage];
		for (i32 texIdx = 0; texIdx < texInfos.size(); ++texIdx)
		{
			const MaterialRawData::TextureBasicInfo& texInfo = texInfos[texIdx];
			Assert(textures.find(texInfo.mTexturePath) != textures.end());
			const auto& p = textures.find(texInfo.mTexturePath)->second;
			result->mTextureParams[texUsage].push_back(p.first);
			result->mSamplerParams[texUsage].push_back(p.second);
		}
	}

	return result;
}
