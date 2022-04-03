#pragma once

#include "World/Scene.h"

class D3D12Texture;
class D3D12CommandContext;
class D3D12SamplerView;

struct RenderMaterial
{
	std::array<std::vector<D3D12Texture*>, TextureUsage_Count> mTextureParams;
	std::array<std::vector<D3D12SamplerView*>, TextureUsage_Count> mSamplerParams;

	void UpdateGpuResources(D3D12CommandContext* context);
	bool IsGpuResourceReady() const;

	static RenderMaterial* GenerateRenderMaterialFromRawData(
		const MaterialRawData* rawData, 
		const std::map<std::string, std::pair<D3D12Texture*, D3D12SamplerView*>>& textures);
};