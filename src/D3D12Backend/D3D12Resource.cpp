#include "D3D12BackendPch.h"
#include "D3D12Resource.h"
#include "D3D12ResourceView.h"

D3D12Backend::CommitedResource* D3D12Backend::CommitedResource::Builder::Build(D3D12Device* device, D3D12_HEAP_TYPE heapType)
{
	CommitedResource* result = new CommitedResource;

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

	// create gpu resource default as copy dest
	ID3D12Resource* resource = nullptr;
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
	AssertHResultOk(device->GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		mInitState,
		nullptr,
		IID_PPV_ARGS(&resource)));

	NAME_RAW_D3D12_OBJECT(resource, mName);

	result->mDevice = device;
	result->mResource = resource;
	result->mSize = { (i32)mWidth, (i32)mHeight, mDepthOrArraySize };
	result->mDesc = desc;
	result->mState = mInitState;

	return result;
}

D3D12Backend::CommitedResource* D3D12Backend::CommitedResource::Possessor::Possess(D3D12Device* device)
{
	CommitedResource* result = new CommitedResource;

	NAME_RAW_D3D12_OBJECT(mResource, mName);
	const D3D12_RESOURCE_DESC& desc = mResource->GetDesc();

	result->mDevice = device;
	result->mResource = mResource;
	result->mSize = { (i32)desc.Width, (i32)desc.Height, desc.DepthOrArraySize };
	result->mDesc = desc;
	result->mState = mCurrentState;

	return result;
}

D3D12Backend::CommitedResource::~CommitedResource()
{
	mDevice->ReleaseD3D12Resource(mResource);
}

void D3D12Backend::CommitedResource::Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState)
{
	if (mState != destState)
	{
		context->Transition(mResource, mState, destState);
		mState = destState;
	}
}

D3D12Backend::CommitedResource::SrvBuilder D3D12Backend::CommitedResource::CreateSrv()
{
	return SrvBuilder().SetDevice(mDevice).SetResource(this);
}

D3D12Backend::CommitedResource::RtvBuilder D3D12Backend::CommitedResource::CreateRtv()
{
	return RtvBuilder().SetDevice(mDevice).SetResource(this);
}

D3D12Backend::CommitedResource::DsvBuilder D3D12Backend::CommitedResource::CreateDsv()
{
	return DsvBuilder().SetDevice(mDevice).SetResource(this);
}

SRV* D3D12Backend::CommitedResource::SrvBuilder::BuildTex2D()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	{
		desc.Format = mFormat;
		desc.ViewDimension = mViewDimension;
		desc.Shader4ComponentMapping = mShader4ComponentMapping;
		desc.Texture2D.MostDetailedMip = mMostDetailedMip;
		desc.Texture2D.MipLevels = mMipLevels;
		desc.Texture2D.PlaneSlice = mPlaneSlice;
		desc.Texture2D.ResourceMinLODClamp = mResourceMinLODClamp;
	}

	return new SRV(mDevice, mResource, desc);
}

RTV* D3D12Backend::CommitedResource::RtvBuilder::BuildTex2D()
{
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	{
		desc.Format = mFormat;
		desc.ViewDimension = mViewDimension;
		desc.Texture2D.MipSlice = mMipSlice;
		desc.Texture2D.PlaneSlice = mPlaneSlice;
	}

	return new RTV(mDevice, mResource, desc);
}

DSV* D3D12Backend::CommitedResource::DsvBuilder::BuildTex2D()
{
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	{
		desc.Format = mFormat;
		desc.ViewDimension = mViewDimension;
		desc.Flags = mFlags;
		desc.Texture2D.MipSlice = mMipSlice;
	}

	return new DSV(mDevice, mResource, desc);
}

D3D12Backend::CommitedResource* D3D12Backend::CreateCommitedResourceTex2D(D3D12Device* device, const Vec3i& size, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initStates, const char* name, u32 mipLevels)
{
	return D3D12Backend::CommitedResource::Builder()
		.SetDimention(D3D12_RESOURCE_DIMENSION_TEXTURE2D)
		.SetWidth(size.x())
		.SetHeight(size.y())
		.SetDepthOrArraySize(size.z())
		.SetMipLevels(mipLevels)
		.SetFormat(format)
		.SetFlags(flags)
		.SetName(name)
		.SetInitState(initStates)
		.BuildDefault(device);
}

