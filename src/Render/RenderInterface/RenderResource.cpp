#include "RenderPch.h"
#include "RenderResource.h"
#include "RenderModule.h"

RenderResourceManager::RenderResourceManager(RenderModule* renderModule)
	: mRenderModule(renderModule)
{}

ResourceId RenderResourceManager::CreateNamedReadonlyResource(const char* name, const std::vector<b8>& data, ResourceInitializer* initializer)
{
	if (mName2ReadonlyResourceId.find(name) == mName2ReadonlyResourceId.end())
	{
		mName2ReadonlyResourceId[name] = mIndexAllocator.Alloc();
	}

	const ResourceId newId = mName2ReadonlyResourceId[name];
	mFileResources[newId] = std::make_unique<Texture>(name, data);
	return ResourceId();
}

ResourceId RenderResourceManager::CreateSwapChainResource()
{
	if (!mSwapChainResourceId.IsValid())
	{
		mSwapChainResourceId = mIndexAllocator.Alloc();
	}
	
	return mSwapChainResourceId;
}

ResourceId RenderResourceManager::CreateTransientResource(const RHI::CommitedResourceDesc& desc)
{
	return ResourceId();
}

void RenderResourceManager::PreRender()
{
	D3D12Backend::GraphicsContext* context = mRenderModule->GetDevice()->GetGpuQueue(D3D12Backend::D3D12GpuQueueType::Graphic)->AllocGraphicContext();

	for (const auto& [id, res] : mFileResources)
	{
		if (!res->IsD3DResourceReady())
		{
			res->Initial(context);
		}
	}
}
