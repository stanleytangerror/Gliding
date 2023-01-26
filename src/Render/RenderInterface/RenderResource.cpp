#include "RenderPch.h"
#include "RenderResource.h"
#include "RenderModule.h"
#include "RenderTypeD3D12Utils.h"
#include "D3D12Backend/D3D12CommandContext.h"
#include "D3D12Backend/D3D12Resource.h"
#include "D3D12Backend/D3D12ResourceView.h"

RenderResourceManager::RenderResourceManager(RenderModule* renderModule)
	: mRenderModule(renderModule)
{}

ResourceId RenderResourceManager::CreateNamedReadonlyResource(const char* name, RenderResourceInitializer* initializer)
{
	if (mName2ReadonlyResourceId.find(name) == mName2ReadonlyResourceId.end())
	{
		mName2ReadonlyResourceId[name] = mResourceIdAllocator.Alloc();
	}

	const ResourceId newId = mName2ReadonlyResourceId[name];
	mNamedReadonlyResourceInitializer[newId] = initializer;
	return newId;
}

ResourceId RenderResourceManager::CreateSwapChainResource()
{
	if (!mSwapChainResourceId.IsValid())
	{
		mSwapChainResourceId = mResourceIdAllocator.Alloc();
	}
	
	return mSwapChainResourceId;
}

ResourceId RenderResourceManager::CreateTransientResource(const RHI::CommitedResourceDesc& desc)
{
	return ResourceId();
}

D3D12Backend::CommitedResource* RenderResourceManager::GetNamedReadonlyResource(ResourceId resourceId) const
{
	auto it = mNamedReadonlyResources.find(resourceId);
	return it != mNamedReadonlyResources.end() ? it->second : nullptr;
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

		D3D12Backend::CommitedResource* resource = it->second;

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
	}
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

		D3D12Backend::CommitedResource* resource = it->second;

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
	}
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

void RenderResourceManager::PreRender()
{
	D3D12Backend::GraphicsContext* context = mRenderModule->GetDevice()->GetGpuQueue(D3D12Backend::D3D12GpuQueueType::Graphic)->AllocGraphicContext();

	for (const auto& [id, initializer] : mNamedReadonlyResourceInitializer)
	{
		mNamedReadonlyResources[id] = initializer->Initialize(context).get();
	}
	mNamedReadonlyResourceInitializer.clear();

	context->Finalize();
}
