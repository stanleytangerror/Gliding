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

RenderMaterial* RenderMaterial::GenerateRenderMaterialFromRawData(const MaterialRawData* rawData, const std::map<std::string, D3D12Texture*>& textures)
{
	RenderMaterial* result = new RenderMaterial;

	for (i32 texUsage = 0; texUsage < TextureUsage_Count; ++texUsage)
	{
		const auto& texPaths = rawData->mTexturesWithUsage[texUsage];
		for (i32 texIdx = 0; texIdx < texPaths.size(); ++texIdx)
		{
			const auto& texPath = texPaths[texIdx];
			Assert(textures.find(texPath) != textures.end());
			result->mTextureParams[texUsage].push_back(textures.find(texPath)->second);
		}
	}

	return result;
}
