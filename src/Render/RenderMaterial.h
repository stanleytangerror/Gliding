#pragma once

#include "Common/ModelProcess.h"
#include "World/Scene.h"
#include "Common/GraphicsInfrastructure.h"
#include "FrameGraph.h"

namespace ModelProcess
{
	struct Material;
}

struct RenderMaterial
{
	struct MaterialAttriSlot
	{
		class FileTexture* mTexture = nullptr;
		GI::SamplerDesc mSampler;
		Vec4f mConstantValue = Vec4f::Zero();
	};

	std::array<MaterialAttriSlot, TextureUsage_Count> mMatAttriSlots;


	struct NormalChannel
	{
		Vec3f mNormalConstant = { 0.f, 0.f, 1.f };
		f32 mNormalScale = 1.f;
		FileTexture* mTexture = nullptr;
		GI::SamplerDesc mSampler;
	};

	struct OcclusionChannel
	{
		f32 mOcclusionStrength = 1.f;
		FileTexture* mTexture = nullptr;
		GI::SamplerDesc mSampler;
	};

	struct EmissiveChannel
	{
		f32 mEmissiveStrength = 1.f;
		FileTexture* mTexture = nullptr;
		GI::SamplerDesc mSampler;
	};

	struct MetallicRoughnessChannel
	{
		f32 mMetallicFactor = 1.f;
		f32 mRoughnessFactor = 1.f;
		FileTexture* mTexture = nullptr;
		GI::SamplerDesc mSampler;
	};

	struct BaseColorChannel
	{
		FileTexture* mTexture = nullptr;
		Vec4f mColor = Vec4f::Zero();
		GI::SamplerDesc mSampler;
	};

	NormalChannel mNormalChannel;
	OcclusionChannel mOcclusionChannel;
	EmissiveChannel mEmissiveChannel;
	MetallicRoughnessChannel mMetallicRoughnessChannel;
	BaseColorChannel mBaseColorChannel;

	static RenderMaterial* GenerateRenderMaterialFromRawData(
		const MaterialRawData* matRawData,
		const SceneRawData* sceneRawData,
		const std::map<std::string, class FileTexture*>& textures,
		const std::map<TextureSamplerType, GI::SamplerDesc>& samplers);

	static RenderMaterial* GenerateRenderMaterialFromMaterialData(
		FrameGraph* frameGraph,
		const ModelProcess::Material& material,
		const std::map<Guid, std::pair<FileTexture*, GI::SamplerDesc>>& textureMap);
};
