#include "D3D12Backend/D3D12BackendPch.h"
#include "D3D12Resource.h"

namespace D3D12Backend
{
	GraphicMemoryResource::GraphicMemoryResource(D3D12Device* device, GI::CommittedResourceId id, const char* name)
		: GI::IGraphicMemoryResource(id)
		, mDevice(device)
		, mDebugName(name ? name : "")
	{

	}


	GraphicMemoryResource::~GraphicMemoryResource()
	{
		mDevice->GetResourceManager()->ReleaseResource(mDeviceResourceId);
	}



	GI::HeapType::Enum GraphicMemoryResource::GetHeapType() const
	{
		return mDevice->GetResourceManager()->GetResource(mDeviceResourceId)->GetHeapType();
	}


	GI::ResourceDimension::Enum GraphicMemoryResource::GetDimension() const
	{
		return mDevice->GetResourceManager()->GetResource(mDeviceResourceId)->GetDimension();
	}


	Vec3u GraphicMemoryResource::GetSize() const
	{
		return mDevice->GetResourceManager()->GetResource(mDeviceResourceId)->GetSize();
	}


	GI::Format::Enum GraphicMemoryResource::GetFormat() const
	{
		return mDevice->GetResourceManager()->GetResource(mDeviceResourceId)->GetFormat();
	}


	u16 GraphicMemoryResource::GetMipLevelCount() const
	{
		return mDevice->GetResourceManager()->GetResource(mDeviceResourceId)->GetMipLevelCount();
	}

	const char* GraphicMemoryResource::GetDebugName() const
	{
		return mDebugName.c_str();
	}

	CommitedResource::~CommitedResource()
	{
		mResource->Release();
	}

	void CommitedResource::Transition(D3D12Backend::D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState)
	{
		if (mState != destState)
		{
			context->Transition(mResource, mState, destState);
			mState = destState;
		}
	}


}