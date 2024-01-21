#include "D3D12BackendPch.h"
#include "D3D12Resource.h"

namespace D3D12Backend
{
	CommitedResource* CommitedResource::Builder::Build(CommittedResourceId id, D3D12Device* device, GI::HeapType::Enum heapType) const
	{
		CommitedResource* result = new CommitedResource(id);

		D3D12_RESOURCE_DESC desc = {};
		{
			desc.Dimension = mDimention;
			desc.Alignment = mAlignment;
			desc.Width = mWidth;
			desc.Height = mHeight;
			desc.DepthOrArraySize = mDepthOrArraySize;
			desc.MipLevels = mMipLevels;
			desc.Format = mFormat;
			desc.SampleDesc = mSampleDesc;
			desc.Layout = mLayout;
			desc.Flags = mFlags;
		}

		ID3D12Resource* resource = device->GetResourceManager()->CreateResource(
			[heapType, desc, this] (ID3D12Device* device) 
			{
				ID3D12Resource* resource = nullptr;
				CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE(heapType));
				AssertHResultOk(device->CreateCommittedResource(
					&heapProp,
					D3D12_HEAP_FLAG_NONE,
					&desc,
					this->mInitState,
					nullptr,
					IID_PPV_ARGS(&resource)));
				NAME_RAW_D3D12_OBJECT(resource, mName);
				return resource;
			});

		result->mDevice = device;
		result->mResource = resource;
		result->mSize = { (i32)mWidth, (i32)mHeight, mDepthOrArraySize };
		result->mDesc = desc;
		result->mState = mInitState;
		result->mHeapType = heapType;

		return result;
	}

	CommitedResource* Possessor::Possess(CommittedResourceId id, D3D12Device* device) const
	{
		CommitedResource* result = new CommitedResource(id);

		NAME_RAW_D3D12_OBJECT(mResource, mName);
		const D3D12_RESOURCE_DESC& desc = mResource->GetDesc();

		result->mDevice = device;
		result->mResource = mResource;
		result->mSize = { (i32)desc.Width, (i32)desc.Height, desc.DepthOrArraySize };
		result->mDesc = desc;
		result->mState = mCurrentState;

		return result;
	}

	CommitedResource::~CommitedResource()
	{
		mDevice->ReleaseD3D12Resource(mResource);
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