#include "Render/RenderPch.h"
#include "RenderMaterial.h"
#include "Texture.h"

RenderMaterial* RenderMaterial::GenerateRenderMaterialFromMaterialData(
	FrameGraph* frameGraph, 
	const ModelProcess::Material& material,
	const std::map<Guid, std::pair<std::shared_ptr<FileTexture>, GI::SamplerDesc>>& textureMap)
{
	RenderMaterial* result = new RenderMaterial;

	for (const auto& channel : material.Channels)
	{
		std::shared_ptr<FileTexture> texture;
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
		else if (std::strcmp(channel.Name.c_str(), "Diffuse") == 0)
		{
			result->mDiffuseChannel.mTexture = texture;
			result->mDiffuseChannel.mSampler = sampler;
			result->mDiffuseChannel.mDiffuseConstant = ModelProcess::GetVec4fParam(channel, "RGBA");
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
		else if (std::strcmp(channel.Name.c_str(), "SpecularGlossiness") == 0)
		{
			result->mSpecularGlossinessChannel.mTexture = texture;
			result->mSpecularGlossinessChannel.mSampler = sampler;
			result->mSpecularGlossinessChannel.mSpecularConstant = ModelProcess::GetVec3fParam(channel, "SpecularFactor");
			result->mSpecularGlossinessChannel.mGlossinessConstant = ModelProcess::GetScalarParam(channel, "GlossinessFactor");
		}
	}
	
	return result;
}
