#pragma once

#include "RenderTypes.h"
#include "Common/IndexAllocator.h"
#include "Common/PresentPort.h"
#include <string>
#include <memory>

class RenderModule;

namespace D3D12Backend
{
	class CommitedResource;
	class ShaderResourceView;
	class RenderTargetView;
	class SamplerView;
	class D3D12CommandContext;
}

struct DummyRenderResource {};
using ResourceId = IndexId<DummyRenderResource>;

struct DummyRenderResourceView {};
using ViewId = IndexId<DummyRenderResourceView>;

struct DummySampler {};
using SamplerId = IndexId<DummySampler>;

class GD_RENDER_API RenderResourceInitializer
{
public:
	virtual std::unique_ptr<D3D12Backend::CommitedResource> Initialize(D3D12Backend::D3D12CommandContext* context) = 0;
};

class TransientResourceInitializer : public RenderResourceInitializer
{
public:
	TransientResourceInitializer(const RHI::CommitedResourceDesc& desc);
	std::unique_ptr<D3D12Backend::CommitedResource> Initialize(D3D12Backend::D3D12CommandContext* context) override;

protected:
	RHI::CommitedResourceDesc mDesc;
};

class SamplerInitializer
{
public:
	SamplerInitializer(const RHI::SamplerDesc& desc);
	D3D12Backend::SamplerView* Initialize(D3D12Backend::D3D12CommandContext* context);

protected:
	RHI::SamplerDesc mDesc;
};

class GD_RENDER_API RenderResourceManager
{
public:
	RenderResourceManager(RenderModule* renderModule);

	RenderResourceManager(const RenderResourceManager&) = delete;
	RenderResourceManager& operator=(const RenderResourceManager&) = delete;

	ResourceId CreateNamedReadonlyResource(const char* name, RenderResourceInitializer* initializer);
	ResourceId CreateSwapChainResource(PresentPortType type);
	ResourceId CreateTransientResource(const RHI::CommitedResourceDesc& desc);

	D3D12Backend::CommitedResource* GetResource(ResourceId resourceId) const;

	ViewId CreateSrv(ResourceId resourceId, const RHI::ShaderResourceViewDesc& desc);
	ViewId CreateRtv(ResourceId resourceId, const RHI::RenderTargetViewDesc& desc);
	
	D3D12Backend::ShaderResourceView* GetSrv(ViewId viewId) const;
	D3D12Backend::RenderTargetView* GetRtv(ViewId viewId) const;

	SamplerId CreateSampler(const RHI::SamplerDesc& desc);

	D3D12Backend::SamplerView* GetSampler(SamplerId samplerId) const;

	void PreRender();

private:
	D3D12Backend::CommitedResource* GetNamedReadonlyResource(ResourceId resourceId) const;
	D3D12Backend::CommitedResource* GetTransientResource(ResourceId resourceId) const;
	D3D12Backend::CommitedResource* GetSwapChainResource(ResourceId resourceId) const;

private:
	RenderModule* mRenderModule = nullptr;
	IndexAllocator<ResourceId> mResourceIdAllocator;
	IndexAllocator<ViewId> mViewIdAllocator;
	IndexAllocator<SamplerId> mSamperIdAllocator;

	std::map<std::string, ResourceId> mName2ReadonlyResourceId;
	std::map<ResourceId, RenderResourceInitializer*> mNamedReadonlyResourceInitializer;
	std::map<ResourceId, std::unique_ptr<D3D12Backend::CommitedResource>>  mNamedReadonlyResources;

	std::map<u32, ResourceId> mTransientHash2ResourceId;
	std::map<ResourceId, TransientResourceInitializer*> mTransientResourceInitializer;
	std::map<ResourceId, std::unique_ptr<D3D12Backend::CommitedResource>>  mTransientResources;

	std::map<u32, ViewId> mSrvHash2ViewId;
	std::map<ViewId, D3D12Backend::ShaderResourceView*> mSrvs;

	std::map<u32, ViewId> mRtvHash2ViewId;
	std::map<ViewId, D3D12Backend::RenderTargetView*> mRtvs;

	std::map<PresentPortType, ResourceId> mSwapChainResourceIds;
	std::map<ResourceId, D3D12Backend::CommitedResource*> mSwapChainResources;

	std::map<u32, SamplerId> mSamplerHash2SamplerId;
	std::map<SamplerId, SamplerInitializer*> mSamplerInitializer;
	std::map<SamplerId, D3D12Backend::SamplerView*> mSamplers;
};
