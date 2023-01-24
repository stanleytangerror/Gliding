#pragma once

#include "RenderTypes.h"
#include "Common/IndexAllocator.h"
#include "Texture.h"
#include <string>

class CommandContext;
class RenderModule;

struct ResourceId
{
	u64 mId = (~0x0);

	static ResourceId FromInt(u64 index) { return { index }; }
	bool IsValid() const { return mId != (~0x0); }
	bool operator<(const ResourceId& other) const { return mId < other.mId; }
};

struct ResourceInitializer
{

};

class GD_RENDER_API RenderResourceManager
{
public:
	RenderResourceManager(RenderModule* renderModule);

	ResourceId CreateNamedReadonlyResource(const char* name, const std::vector<b8>& data, ResourceInitializer* initializer);
	ResourceId CreateSwapChainResource();
	ResourceId CreateTransientResource(const RHI::CommitedResourceDesc& desc);

	void PreRender();

private:
	RenderModule* mRenderModule = nullptr;
	IndexAllocator<ResourceId> mIndexAllocator;

	std::map<std::string, ResourceId> mName2ReadonlyResourceId;
	std::map<ResourceId, std::unique_ptr<Texture>>  mFileResources;

	ResourceId mSwapChainResourceId;
};
