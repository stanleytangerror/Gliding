#include "D3D12BackendPch.h"
#include "D3D12RenderTarget.h"


D3D12RenderTarget::D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, const char* name)
	: D3D12RenderTarget(device, size, format, 1, name)
{
}

D3D12RenderTarget::D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, i32 mipLevelCount, const char* name)
	: mDevice(device)
	, mSize(size)
	, mMipLevelCount(mipLevelCount)
	, mFormat(format)
	, mState(D3D12_RESOURCE_STATE_RENDER_TARGET)
{
	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	{
		textureDesc.MipLevels = mipLevelCount;
		textureDesc.Format = mFormat;
		textureDesc.Width = mSize.x();
		textureDesc.Height = mSize.y();
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		textureDesc.DepthOrArraySize = mSize.z();
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	}

	// create gpu resource default as copy dest
	const CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT };
	AssertHResultOk(device->GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		mState,
		nullptr,
		IID_PPV_ARGS(&mResource)));

	NAME_RAW_D3D12_OBJECT(mResource, name);

	mSrv = new SRV(mDevice, this);
	mRtv = CreateTex2DRtv();
	mUav = new UAV(mDevice, this);
}

D3D12RenderTarget::D3D12RenderTarget(D3D12Device* device, i32 count, i32 stride, DXGI_FORMAT format, const char* name)
	: mDevice(device)
	, mSize(count * stride, 1, 1)
	, mFormat(format)
	, mState(D3D12_RESOURCE_STATE_RENDER_TARGET)
{
	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC desc = {};
	{
		desc.MipLevels = 1;
		desc.Format = mFormat;
		desc.Width = mSize.x();
		desc.Height = mSize.y();
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		desc.DepthOrArraySize = mSize.z();
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	}

	// create gpu resource default as copy dest
	const CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT };
	AssertHResultOk(device->GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		mState,
		nullptr,
		IID_PPV_ARGS(&mResource)));

	NAME_RAW_D3D12_OBJECT(mResource, name);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	{
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12Utils::GetSrvDimension(D3D12_RESOURCE_DIMENSION_BUFFER);
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = count;
		srvDesc.Buffer.StructureByteStride = stride;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	}

	mSrv = new SRV(mDevice, this, srvDesc);


	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	{
		uavDesc.Format = format;
		uavDesc.ViewDimension = D3D12Utils::GetUavDimension(D3D12_RESOURCE_DIMENSION_BUFFER);
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = count;
		uavDesc.Buffer.StructureByteStride = stride;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	}

	mUav = new UAV(mDevice, this, uavDesc);
}

D3D12RenderTarget::~D3D12RenderTarget()
{
	mDevice->ReleaseD3D12Resource(mResource);
}

void D3D12RenderTarget::Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState)
{
	if (mState != destState)
	{
		context->Transition(mResource, mState, destState);
		mState = destState;
	}
}

RTV* D3D12RenderTarget::CreateTex2DRtv()
{
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	{
		desc.Format = mFormat;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		desc.Texture2D.PlaneSlice = 0;
	}

	return new RTV(mDevice, this, desc);
}

RTV* D3D12RenderTarget::CreateTex2DArrayRtv(uint32_t firstArrayIdx, uint32_t arrayCount)
{
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	{
		desc.Format = mFormat;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipSlice = 0;
		desc.Texture2DArray.FirstArraySlice = firstArrayIdx;
		desc.Texture2DArray.ArraySize = arrayCount;
		desc.Texture2DArray.PlaneSlice = 0;
	}

	return new RTV(mDevice, this, desc);
}

SRV* D3D12RenderTarget::CreateTexCubeSrv()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	{
		desc.Format = mFormat;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.TextureCube.MipLevels = -1;
		desc.TextureCube.ResourceMinLODClamp = 0.f;
		desc.TextureCube.MostDetailedMip = 0;
	}

	return new SRV(mDevice, this, desc);
}

//DSV* D3D12RenderTarget::CreateTex2DDsv()
//{
//	return new DSV(this);
//}

void D3D12RenderTarget::Clear(D3D12CommandContext* context, const Vec4f& color)
{
	Transition(context, D3D12_RESOURCE_STATE_RENDER_TARGET);

	using F4 = const FLOAT[4];
	context->GetCommandList()->ClearRenderTargetView(GetRtv()->GetHandle(), *(F4*)&color, 0, nullptr);
}

//////////////////////////////////////////////

D3DDepthStencil::D3DDepthStencil(D3D12Device* device, Vec2i size, DXGI_FORMAT format, DXGI_FORMAT dsvFormat, DXGI_FORMAT srvFormat, const char* name)
	: mDevice(device)
	, mSize{ size.x(), size.y(), 1 }
	, mFormat(format)
	, mState(D3D12_RESOURCE_STATE_DEPTH_WRITE)
{
	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	{
		textureDesc.MipLevels = 1;
		textureDesc.Format = mFormat;
		textureDesc.Width = mSize.x();
		textureDesc.Height = mSize.y();
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	}

	// create gpu resource default as copy dest
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	AssertHResultOk(device->GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		mState,
		nullptr,
		IID_PPV_ARGS(&mResource)));

	NAME_RAW_D3D12_OBJECT(mResource, name);

	mSrv = new SRV(mDevice, this, srvFormat);

	mDsv = new DSV(mDevice, this, dsvFormat);
}

D3DDepthStencil::~D3DDepthStencil()
{
	mDevice->ReleaseD3D12Resource(mResource);
}

void D3DDepthStencil::Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState)
{
	if (mState != destState)
	{
		context->Transition(mResource, mState, destState);
		mState = destState;
	}
}

void D3DDepthStencil::Clear(D3D12CommandContext* context, float depth, UINT stencil)
{
	Transition(context, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	context->GetCommandList()->ClearDepthStencilView(GetDsv()->GetHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}

D3D12Backend::CommitedResource* D3D12Backend::CommitedResource::Builder::Build(D3D12Device* device)
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
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET;
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	AssertHResultOk(device->GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		state,
		nullptr,
		IID_PPV_ARGS(&resource)));

	NAME_RAW_D3D12_OBJECT(resource, mName);

	result->mDevice = device;
	result->mResource = resource;
	result->mSize = { (i32) mWidth, (i32) mHeight, mDepthOrArraySize };
	result->mMipLevelCount = mMipLevels;
	result->mFormat = mFormat;
	result->mState = state;
	
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
