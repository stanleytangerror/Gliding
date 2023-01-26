#pragma once

#include "RenderTypes.h"
#include "Common/IndexAllocator.h"
#include <string>

class RenderModule;

namespace D3D12Backend
{
	class CommitedResource;
	class ShaderResourceView;
	class RenderTargetView;
	class D3D12CommandContext;
}

struct DummyRenderResource {};
using ResourceId = IndexId<DummyRenderResource>;

struct DummyRenderResourceView {};
using ViewId = IndexId<DummyRenderResourceView>;

class GD_RENDER_API RenderResourceInitializer
{
public:
	virtual std::unique_ptr<D3D12Backend::CommitedResource> Initialize(D3D12Backend::D3D12CommandContext* context) = 0;
};

class GD_RENDER_API RenderResourceManager
{
public:
	RenderResourceManager(RenderModule* renderModule);

	ResourceId CreateNamedReadonlyResource(const char* name, RenderResourceInitializer* initializer);
	ResourceId CreateSwapChainResource();
	ResourceId CreateTransientResource(const RHI::CommitedResourceDesc& desc);

	D3D12Backend::CommitedResource* GetNamedReadonlyResource(ResourceId resourceId) const;

	ViewId CreateSrv(ResourceId resourceId, const RHI::ShaderResourceViewDesc& desc);
	ViewId CreateRtv(ResourceId resourceId, const RHI::RenderTargetViewDesc& desc);
	
	D3D12Backend::ShaderResourceView* GetSrv(ViewId viewId) const;
	D3D12Backend::RenderTargetView* GetRtv(ViewId viewId) const;

	void PreRender();

private:
	RenderModule* mRenderModule = nullptr;
	IndexAllocator<ResourceId> mResourceIdAllocator;
	IndexAllocator<ViewId> mViewIdAllocator;

	std::map<std::string, ResourceId> mName2ReadonlyResourceId;
	std::map<ResourceId, RenderResourceInitializer*> mNamedReadonlyResourceInitializer;
	std::map<ResourceId, D3D12Backend::CommitedResource*>  mNamedReadonlyResources;

	std::map<u32, ViewId> mSrvHash2ViewId;
	std::map<ViewId, D3D12Backend::ShaderResourceView*> mSrvs;

	std::map<u32, ViewId> mRtvHash2ViewId;
	std::map<ViewId, D3D12Backend::RenderTargetView*> mRtvs;

	ResourceId mSwapChainResourceId;
};
