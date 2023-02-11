#include "RenderPch.h"
#include "RenderResource.h"
#include "RenderModule.h"
#include "Common/PresentPort.h"
#include "RenderTypeD3D12Utils.h"
#include "D3D12Backend/D3D12CommandContext.h"
#include "D3D12Backend/D3D12Resource.h"
#include "D3D12Backend/D3D12ResourceView.h"
#include "D3D12Backend/D3D12SwapChain.h"

RenderResourceManager::RenderResourceManager(RenderModule* renderModule)
	: mRenderModule(renderModule)
{}

ResourceId RenderResourceManager::CreateNamedReadonlyResource(const char* name, RenderResourceInitializer* initializer)
{
	if (mName2ReadonlyResourceId.find(name) == mName2ReadonlyResourceId.end())
	{
		const auto newId = mResourceIdAllocator.Alloc();
		
		mName2ReadonlyResourceId[name] = newId;
		mNamedReadonlyResourceInitializer[newId] = initializer;
	}

	return mName2ReadonlyResourceId[name];
}

ResourceId RenderResourceManager::CreateSwapChainResource(PresentPortType type)
{
	if (mSwapChainResourceIds.find(type) == mSwapChainResourceIds.end())
	{
		mSwapChainResourceIds[type] = mResourceIdAllocator.Alloc();
	}
	
	return mSwapChainResourceIds[type];
}

ResourceId RenderResourceManager::CreateTransientResource(const RHI::CommitedResourceDesc& desc)
{
	auto hash = Utils::HashBytes(&desc);
	if (mTransientHash2ResourceId.find(hash) == mTransientHash2ResourceId.end())
	{
		const auto newId = mResourceIdAllocator.Alloc();
		mTransientResourceInitializer[newId] = new TransientResourceInitializer(desc);
	}

	return mTransientHash2ResourceId[hash];
}

D3D12Backend::CommitedResource* RenderResourceManager::GetResource(ResourceId resourceId) const
{
	auto resource = GetNamedReadonlyResource(resourceId);
	if (resource) { return resource; }

	resource = GetTransientResource(resourceId);
	if (resource) { return resource; }

	resource = GetSwapChainResource(resourceId);
	return resource;
}

D3D12Backend::CommitedResource* RenderResourceManager::GetNamedReadonlyResource(ResourceId resourceId) const
{
	auto it = mNamedReadonlyResources.find(resourceId);
	return it != mNamedReadonlyResources.end() ? it->second.get() : nullptr;
}

D3D12Backend::CommitedResource* RenderResourceManager::GetTransientResource(ResourceId resourceId) const
{
	auto it = mTransientResources.find(resourceId);
	return it != mTransientResources.end() ? it->second.get() : nullptr;
}


D3D12Backend::CommitedResource* RenderResourceManager::GetSwapChainResource(ResourceId resourceId) const
{
	auto it = mSwapChainResources.find(resourceId);
	return it != mSwapChainResources.end() ? it->second : nullptr;
}

ViewId RenderResourceManager::CreateSrv(ResourceId resourceId, const RHI::ShaderResourceViewDesc& desc)
{
	auto it = mNamedReadonlyResources.find(resourceId);
	if (it != mNamedReadonlyResources.end())
	{
		auto h = Utils::HashBytes(&desc);
		if (mSrvHash2ViewId.find(h) != mSrvHash2ViewId.end())
		{
			return mSrvHash2ViewId[h];
		}

		D3D12Backend::CommitedResource* resource = it->second.get();

		auto& builder = resource->CreateSrv()
			.SetFormat(ToDxgiFormat(desc.Format))
			.SetViewDimension(ToSrvDimension(desc.Dimension));

		switch (desc.Dimension)
		{
			case RHI::ViewDimension::BUFFER 						  : builder.SetBuffer						  (ToSrvDesc(desc.Buffer						   )); break;
			case RHI::ViewDimension::TEXTURE1D 						  : builder.SetTexture1D					  (ToSrvDesc(desc.Texture1D						   )); break;
			case RHI::ViewDimension::TEXTURE1DARRAY 				  : builder.SetTexture1DArray				  (ToSrvDesc(desc.Texture1DArray				   )); break;
			case RHI::ViewDimension::TEXTURE2D 						  : builder.SetTexture2D					  (ToSrvDesc(desc.Texture2D						   )); break;
			case RHI::ViewDimension::TEXTURE2DARRAY 				  : builder.SetTexture2DArray				  (ToSrvDesc(desc.Texture2DArray				   )); break;
			case RHI::ViewDimension::TEXTURE2DMS 					  : builder.SetTexture2DMS					  (ToSrvDesc(desc.Texture2DMS					   )); break;
			case RHI::ViewDimension::TEXTURE2DMSARRAY 				  : builder.SetTexture2DMSArray				  (ToSrvDesc(desc.Texture2DMSArray			       )); break;
			case RHI::ViewDimension::TEXTURE3D 						  : builder.SetTexture3D					  (ToSrvDesc(desc.Texture3D					       )); break;
			case RHI::ViewDimension::TEXTURECUBE 					  : builder.SetTextureCube					  (ToSrvDesc(desc.TextureCube					   )); break;
			case RHI::ViewDimension::TEXTURECUBEARRAY 				  : builder.SetTextureCubeArray				  (ToSrvDesc(desc.TextureCubeArray			       )); break;
			case RHI::ViewDimension::RAYTRACING_ACCELERATION_STRUCTURE: builder.SetRaytracingAccelerationStructure(ToSrvDesc(desc.RaytracingAccelerationStructure  )); break;
		}

		D3D12Backend::ShaderResourceView* view = builder.Build();

		const auto& viewId = mViewIdAllocator.Alloc();
		mSrvs[viewId] = view;
		mSrvHash2ViewId[h] = viewId;

		return viewId;
	}

	return ViewId::InvalidId();
}

ViewId RenderResourceManager::CreateRtv(ResourceId resourceId, const RHI::RenderTargetViewDesc& desc)
{
	auto it = mNamedReadonlyResources.find(resourceId);
	if (it != mNamedReadonlyResources.end())
	{
		auto h = Utils::HashBytes(&desc);
		if (mRtvHash2ViewId.find(h) != mRtvHash2ViewId.end())
		{
			return mRtvHash2ViewId[h];
		}

		D3D12Backend::CommitedResource* resource = it->second.get();

		auto& builder = resource->CreateRtv()
			.SetFormat(ToDxgiFormat(desc.Format))
			.SetViewDimension(ToRtvDimension(desc.Dimension));

		switch (desc.Dimension)
		{
			case RHI::ViewDimension::BUFFER 						  : builder.SetBuffer						  (ToRtvDesc(desc.Buffer						   )); break;
			case RHI::ViewDimension::TEXTURE1D 						  : builder.SetTexture1D					  (ToRtvDesc(desc.Texture1D						   )); break;
			case RHI::ViewDimension::TEXTURE1DARRAY 				  : builder.SetTexture1DArray				  (ToRtvDesc(desc.Texture1DArray				   )); break;
			case RHI::ViewDimension::TEXTURE2D 						  : builder.SetTexture2D					  (ToRtvDesc(desc.Texture2D						   )); break;
			case RHI::ViewDimension::TEXTURE2DARRAY 				  : builder.SetTexture2DArray				  (ToRtvDesc(desc.Texture2DArray				   )); break;
			case RHI::ViewDimension::TEXTURE2DMS 					  : builder.SetTexture2DMS					  (ToRtvDesc(desc.Texture2DMS					   )); break;
			case RHI::ViewDimension::TEXTURE2DMSARRAY 				  : builder.SetTexture2DMSArray				  (ToRtvDesc(desc.Texture2DMSArray			       )); break;
			case RHI::ViewDimension::TEXTURE3D 						  : builder.SetTexture3D					  (ToRtvDesc(desc.Texture3D					       )); break;
		}

		D3D12Backend::RenderTargetView* view = builder.Build();

		const auto& viewId = mViewIdAllocator.Alloc();
		mRtvs[viewId] = view;
		mRtvHash2ViewId[h] = viewId;

		return viewId;
	}

	return ViewId::InvalidId();
}

D3D12Backend::ShaderResourceView* RenderResourceManager::GetSrv(ViewId viewId) const
{
	auto it = mSrvs.find(viewId);
	return it != mSrvs.end() ? it->second : nullptr;
}

D3D12Backend::RenderTargetView* RenderResourceManager::GetRtv(ViewId viewId) const
{
	auto it = mRtvs.find(viewId);
	return it != mRtvs.end() ? it->second : nullptr;
}


SamplerId RenderResourceManager::CreateSampler(const RHI::SamplerDesc& desc)
{
	auto h = Utils::HashBytes(&desc);
	if (mSamplerHash2SamplerId.find(h) == mSamplerHash2SamplerId.end())
	{
		const auto newId = mSamperIdAllocator.Alloc();
		mSamplerHash2SamplerId[h] = newId;
		mSamplerInitializer[newId] = new SamplerInitializer(desc);
	}

	return mSamplerHash2SamplerId[h];
}

D3D12Backend::SamplerView* RenderResourceManager::GetSampler(SamplerId samplerId) const
{
	auto it = mSamplers.find(samplerId);
	return it != mSamplers.end() ? it->second : nullptr;
}

void RenderResourceManager::PreRender()
{
	D3D12Backend::GraphicsContext* context = mRenderModule->GetDevice()->GetGpuQueue(D3D12Backend::D3D12GpuQueueType::Graphic)->AllocGraphicContext();

	for (const auto& [id, initializer] : mNamedReadonlyResourceInitializer)
	{
		mNamedReadonlyResources.emplace(id, initializer->Initialize(context));
	}
	mNamedReadonlyResourceInitializer.clear();

	for (const auto& [id, initializer] : mTransientResourceInitializer)
	{
		mTransientResources.emplace(id, initializer->Initialize(context));
	}
	mTransientResourceInitializer.clear();

	mSwapChainResources.clear();
	for (const auto& [port, id] : mSwapChainResourceIds)
	{
		mSwapChainResources[id] = context->GetDevice()->GetPresentPort(port).mSwapChain->GetBuffer()->GetResource();
	}

	for (const auto& [id, initializer] : mSamplerInitializer)
	{
		mSamplers[id] = initializer->Initialize(context);
	}
	mSamplerInitializer.clear();

	context->Finalize();
}

TransientResourceInitializer::TransientResourceInitializer(const RHI::CommitedResourceDesc& mDesc)
	: mDesc(mDesc)
{

}

std::unique_ptr<D3D12Backend::CommitedResource> TransientResourceInitializer::Initialize(D3D12Backend::D3D12CommandContext* context)
{
	auto result = D3D12Backend::CommitedResource::Builder()
		.SetDimention(ToResourceDimension(mDesc.Dimension))
		.SetAlignment(0)
		.SetWidth(mDesc.Size.Width)
		.SetHeight(mDesc.Size.Height)
		.SetDepthOrArraySize(mDesc.Size.DepthOrArraySize)
		.SetMipLevels(mDesc.MipLevels)
		.SetFormat(ToDxgiFormat(mDesc.Format))
		.SetFlags(ToResourceFlags(mDesc.Flags))
		.SetInitState(D3D12_RESOURCE_STATE_COMMON)
		.BuildDefault(context->GetDevice());

	Assert(result != nullptr);
	return std::unique_ptr<D3D12Backend::CommitedResource>(result);
}

SamplerInitializer::SamplerInitializer(const RHI::SamplerDesc& desc)
	: mDesc(desc)
{

}

D3D12Backend::SamplerView* SamplerInitializer::Initialize(D3D12Backend::D3D12CommandContext* context)
{
	auto result = new D3D12Backend::SamplerView(context->GetDevice(),
		D3D12_SAMPLER_DESC
		{
			ToFilter(mDesc.Filter),
			ToRextureAddressMode(mDesc.AddressU),
			ToRextureAddressMode(mDesc.AddressV),
			ToRextureAddressMode(mDesc.AddressW),
			mDesc.MipLODBias,
			mDesc.MaxAnisotropy,
			ToComparisonFunc(mDesc.CompFunc),
			{ mDesc.BorderColor.x(), mDesc.BorderColor.y(), mDesc.BorderColor.z(), mDesc.BorderColor.w() },
			mDesc.LODRange.x(),
			mDesc.LODRange.y()
		});

	Assert(result != nullptr);
	return result;
}
