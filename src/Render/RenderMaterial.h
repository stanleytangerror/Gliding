#pragma once

#include "World/Scene.h"

class D3D12Texture;
class D3D12CommandContext;

struct RenderMaterial
{
	std::array<std::vector<D3D12Texture*>, TextureUsage_Count> mTextureParams;

	void UpdateGpuResources(D3D12CommandContext* context);
	bool IsGpuResourceReady() const;

	static RenderMaterial* GenerateRenderMaterialFromRawData(const MaterialRawData* rawData, const std::map<std::string, D3D12Texture*>& textures);
};