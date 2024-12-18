#include "Render/RenderPch.h"
#include "RenderMaterial.h"
#include "Texture.h"

RenderMaterial* RenderMaterial::GenerateRenderMaterialFromMaterialData(
	FrameGraph* frameGraph, 
	const ModelProcess::Material& material,
	const std::map<Guid, std::pair<FileTexture*, GI::SamplerDesc>>& textureMap)
{
	RenderMaterial* result = new RenderMaterial;

	for (const auto& channel : material.Channels)
	{
		FileTexture* texture = nullptr;
		GI::SamplerDesc sampler;
		auto it = textureMap.find(channel.TextureId);
		if (it != textureMap.end())
		{
			texture = it->second.first;
			sampler = it->second.second;
		}

		if (std::strcmp(channel.Name.c_str(), "BaseColor") == 0)
		{
			result->mBaseColorChannel.mTexture = texture;
			result->mBaseColorChannel.mSampler = sampler;
			result->mBaseColorChannel.mColor = ModelProcess::GetVec4fParam(channel, "RGBA");
		}
		else if (std::strcmp(channel.Name.c_str(), "Normal") == 0)
		{
			result->mNormalChannel.mTexture = texture;
			result->mNormalChannel.mSampler = sampler;
			result->mNormalChannel.mNormalScale = ModelProcess::GetScalarParam(channel, "NormalScale");
		}
		else if (std::strcmp(channel.Name.c_str(), "Occlusion") == 0)
		{
			result->mOcclusionChannel.mTexture = texture;
			result->mOcclusionChannel.mSampler = sampler;
			result->mOcclusionChannel.mOcclusionStrength = ModelProcess::GetScalarParam(channel, "OcclusionStrength");
		}
		else if (std::strcmp(channel.Name.c_str(), "Emissive") == 0)
		{
			result->mEmissiveChannel.mTexture = texture;
			result->mEmissiveChannel.mSampler = sampler;
			result->mEmissiveChannel.mEmissiveStrength = ModelProcess::GetScalarParam(channel, "EmissiveStrength");
		}
		else if (std::strcmp(channel.Name.c_str(), "MetallicRoughness") == 0)
		{
			result->mMetallicRoughnessChannel.mTexture = texture;
			result->mMetallicRoughnessChannel.mSampler = sampler;
			result->mMetallicRoughnessChannel.mMetallicFactor = ModelProcess::GetScalarParam(channel, "MetallicFactor");
			result->mMetallicRoughnessChannel.mRoughnessFactor = ModelProcess::GetScalarParam(channel, "RoughnessFactor");
		}
	}
	
	return result;
}
