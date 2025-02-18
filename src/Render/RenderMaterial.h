#pragma once

#include "Common/ModelProcess.h"
#include "Common/GraphicsInfrastructure.h"
#include "FrameGraph.h"
#include "Texture.h"

namespace ModelProcess
{
	struct Material;
}

struct RenderMaterial
{
	struct DiffuseChannel
	{
		Vec4f mDiffuseConstant = Vec4f::Ones();
		std::shared_ptr<FileTexture> mTexture;
		GI::SamplerDesc mSampler;
	};

	struct NormalChannel
	{
		Vec3f mNormalConstant = { 0.f, 0.f, 1.f };
		f32 mNormalScale = 1.f;
		std::shared_ptr<FileTexture> mTexture;
		GI::SamplerDesc mSampler;
	};

	struct OcclusionChannel
	{
		f32 mOcclusionStrength = 1.f;
		std::shared_ptr<FileTexture> mTexture;
		GI::SamplerDesc mSampler;
	};

	struct EmissiveChannel
	{
		f32 mEmissiveStrength = 1.f;
		std::shared_ptr<FileTexture> mTexture;
		GI::SamplerDesc mSampler;
	};

	struct MetallicRoughnessChannel
	{
		f32 mMetallicFactor = 1.f;
		f32 mRoughnessFactor = 1.f;
		std::shared_ptr<FileTexture> mTexture;
		GI::SamplerDesc mSampler;
	};

	struct SpecularGlossinessChannel
	{
		Vec3f mSpecularConstant = Vec3f::Zero();
		f32 mGlossinessConstant = 0.f;
		std::shared_ptr<FileTexture> mTexture;
		GI::SamplerDesc mSampler;
	};

	struct BaseColorChannel
	{
		std::shared_ptr<FileTexture> mTexture;
		Vec4f mColor = Vec4f::Ones();
		GI::SamplerDesc mSampler;
	};

	DiffuseChannel mDiffuseChannel;
	NormalChannel mNormalChannel;
	OcclusionChannel mOcclusionChannel;
	EmissiveChannel mEmissiveChannel;
	MetallicRoughnessChannel mMetallicRoughnessChannel;
	SpecularGlossinessChannel mSpecularGlossinessChannel;
	BaseColorChannel mBaseColorChannel;

	static RenderMaterial* GenerateRenderMaterialFromMaterialData(
		FrameGraph* frameGraph,
		const ModelProcess::Material& material,
		const std::map<Guid, std::pair<std::shared_ptr<FileTexture>, GI::SamplerDesc>>& textureMap);
};
