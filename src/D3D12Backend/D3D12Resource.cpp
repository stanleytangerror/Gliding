#include "D3D12BackendPch.h"
#include "D3D12Resource.h"
#include "D3D12ResourceView.h"

namespace D3D12Backend
{
	CommitedResource* CommitedResource::Builder::Build(D3D12Device* device, D3D12_HEAP_TYPE heapType)
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

	CommitedResource* CommitedResource::Possessor::Possess(D3D12Device* device)
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

	CommitedResource::SrvBuilder CommitedResource::CreateSrv()
	{
		return SrvBuilder().SetDevice(mDevice).SetResource(this);
	}

	CommitedResource::RtvBuilder CommitedResource::CreateRtv()
	{
		return RtvBuilder().SetDevice(mDevice).SetResource(this);
	}

	CommitedResource::DsvBuilder CommitedResource::CreateDsv()
	{
		return DsvBuilder().SetDevice(mDevice).SetResource(this);
	}

	CommitedResource::UavBuilder CommitedResource::CreateUav()
	{
		return UavBuilder().SetDevice(mDevice).SetResource(this);
	}

	ShaderResourceView* CommitedResource::SrvBuilder::Build()
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		{
			desc.Format = mFormat;
			desc.ViewDimension = mViewDimension;
			desc.Shader4ComponentMapping = mShader4ComponentMapping;
			switch (mViewDimension)
			{
			case D3D12_SRV_DIMENSION_BUFFER							  : desc.Buffer							 = mBuffer						   ; break;
			case D3D12_SRV_DIMENSION_TEXTURE1D						  : desc.Texture1D						 = mTexture1D					   ; break;
			case D3D12_SRV_DIMENSION_TEXTURE1DARRAY					  : desc.Texture1DArray					 = mTexture1DArray				   ; break;
			case D3D12_SRV_DIMENSION_TEXTURE2D						  : desc.Texture2D						 = mTexture2D					   ; break;
			case D3D12_SRV_DIMENSION_TEXTURE2DARRAY					  : desc.Texture2DArray					 = mTexture2DArray				   ; break;
			case D3D12_SRV_DIMENSION_TEXTURE2DMS					  : desc.Texture2DMS					 = mTexture2DMS					   ; break;
			case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY				  : desc.Texture2DMSArray				 = mTexture2DMSArray			   ; break;
			case D3D12_SRV_DIMENSION_TEXTURE3D						  : desc.Texture3D						 = mTexture3D					   ; break;
			case D3D12_SRV_DIMENSION_TEXTURECUBE					  : desc.TextureCube					 = mTextureCube					   ; break;
			case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY				  : desc.TextureCubeArray				 = mTextureCubeArray			   ; break;
			case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE: desc.RaytracingAccelerationStructure = mRaytracingAccelerationStructure; break;
			}
		}
	
		return new ShaderResourceView(mDevice, mResource, desc);
	}

	RenderTargetView* CommitedResource::RtvBuilder::Build()
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		{
			desc.Format = mFormat;
			desc.ViewDimension = mViewDimension;
			switch (mViewDimension)
			{
			case D3D12_RTV_DIMENSION_BUFFER							  : desc.Buffer							 = mBuffer						   ; break;
			case D3D12_RTV_DIMENSION_TEXTURE1D						  : desc.Texture1D						 = mTexture1D					   ; break;
			case D3D12_RTV_DIMENSION_TEXTURE1DARRAY					  : desc.Texture1DArray					 = mTexture1DArray				   ; break;
			case D3D12_RTV_DIMENSION_TEXTURE2D						  : desc.Texture2D						 = mTexture2D					   ; break;
			case D3D12_RTV_DIMENSION_TEXTURE2DARRAY					  : desc.Texture2DArray					 = mTexture2DArray				   ; break;
			case D3D12_RTV_DIMENSION_TEXTURE2DMS					  : desc.Texture2DMS					 = mTexture2DMS					   ; break;
			case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY				  : desc.Texture2DMSArray				 = mTexture2DMSArray			   ; break;
			case D3D12_RTV_DIMENSION_TEXTURE3D						  : desc.Texture3D						 = mTexture3D					   ; break;
			}
		}

		return new RenderTargetView(mDevice, mResource, desc);
	}

	DepthStencilView* CommitedResource::DsvBuilder::BuildTex2D()
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
		{
			desc.Format = mFormat;
			desc.ViewDimension = mViewDimension;
			desc.Flags = mFlags;
			desc.Texture2D.MipSlice = mMipSlice;
		}

		return new DepthStencilView(mDevice, mResource, desc);
	}

	UnorderedAccessView* CommitedResource::UavBuilder::BuildBuffer()
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
		{
			desc.Format = mFormat;
			desc.ViewDimension = mViewDimension;
			desc.Buffer.FirstElement = mBuffer_FirstElement;
			desc.Buffer.NumElements = mBuffer_NumElements;
			desc.Buffer.StructureByteStride = mBuffer_StructureByteStride;
			desc.Buffer.CounterOffsetInBytes = mBuffer_CounterOffsetInBytes;
			desc.Buffer.Flags = mBuffer_Flags;
		}

		return new UnorderedAccessView(mDevice, mResource, desc);
	}

	UnorderedAccessView* CommitedResource::UavBuilder::BuildTex2D()
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
		{
			desc.Format = mFormat;
			desc.ViewDimension = mViewDimension;
			desc.Texture2D.MipSlice = mTexture2D_MipSlice;
			desc.Texture2D.PlaneSlice = mTexture2D_PlaneSlice;
		}

		return new UnorderedAccessView(mDevice, mResource, desc);
	}

	CommitedResource* CreateCommitedResourceTex2D(D3D12Device* device, const Vec3i& size, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initStates, const char* name, u32 mipLevels)
	{
		return CommitedResource::Builder()
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
}