#pragma once

#include "World/Scene.h"

class Texture;
namespace D3D12Backend
{
	class D3D12CommandContext;
	class SamplerView;
}

struct RenderMaterial
{
	struct MaterialAttriSlot
	{
		Texture* mTexture = nullptr;
		D3D12Backend::SamplerView* mSampler = nullptr;
		Vec4f mConstantValue = Vec4f::Zero();
	};

	std::array<MaterialAttriSlot, TextureUsage_Count> mMatAttriSlots;

	void UpdateGpuResources(D3D12Backend::D3D12CommandContext* context);
	bool IsGpuResourceReady() const;

	static RenderMaterial* GenerateRenderMaterialFromRawData(
		const MaterialRawData* matRawData,
		const SceneRawData* sceneRawData,
		const std::map<std::string, Texture*>& textures,
		const std::map<TextureSamplerType, D3D12Backend::SamplerView*>& samplers);
};