#pragma once

#include "World/Scene.h"

class D3D12Texture;
class D3D12CommandContext;
namespace D3D12Backend
{
	class SamplerView;
}

struct RenderMaterial
{
	struct MaterialAttriSlot
	{
		D3D12Texture* mTexture = nullptr;
		D3D12Backend::SamplerView* mSampler = nullptr;
		Vec4f mConstantValue = Vec4f::Zero();
	};

	std::array<MaterialAttriSlot, TextureUsage_Count> mMatAttriSlots;

	void UpdateGpuResources(D3D12CommandContext* context);
	bool IsGpuResourceReady() const;

	static RenderMaterial* GenerateRenderMaterialFromRawData(
		const MaterialRawData* matRawData,
		const SceneRawData* sceneRawData,
		const std::map<std::string, D3D12Texture*>& textures,
		const std::map<TextureSamplerType, D3D12Backend::SamplerView*>& samplers);
};