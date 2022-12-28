#include "D3D12BackendPch.h"
#include "D3D12RenderTarget.h"
#include "D3D12Resource.h"

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
	mResource = std::unique_ptr< D3D12Backend::CommitedResource>(D3D12Backend::CommitedResource::Builder()
		.SetDimention(D3D12_RESOURCE_DIMENSION_TEXTURE2D)
		.SetFormat(mFormat)
		.SetMipLevels(mipLevelCount)
		.SetWidth(mSize.x())
		.SetHeight(mSize.y())
		.SetDepthOrArraySize(mSize.z())
		.SetName(name)
		.SetFlags(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
		.BuildDefault(device));

	mSrv = mResource->CreateSrv()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12_SRV_DIMENSION_TEXTURE2D)
		.SetTexture2D_MipLevels(mipLevelCount)
		.BuildTex2D();

	mRtv = mResource->CreateRtv()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12_RTV_DIMENSION_TEXTURE2D)
		.BuildTex2D();

	mUav = mResource->CreateUav()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12_UAV_DIMENSION_TEXTURE2D)
		.BuildTex2D();
}

D3D12RenderTarget::D3D12RenderTarget(D3D12Device* device, i32 count, i32 stride, DXGI_FORMAT format, const char* name)
	: mDevice(device)
	, mSize(count * stride, 1, 1)
	, mFormat(format)
	, mState(D3D12_RESOURCE_STATE_RENDER_TARGET)
{
	mResource = std::unique_ptr< D3D12Backend::CommitedResource>(D3D12Backend::CommitedResource::Builder()
		.SetDimention(D3D12_RESOURCE_DIMENSION_BUFFER)
		.SetFormat(mFormat)
		.SetMipLevels(1)
		.SetWidth(mSize.x())
		.SetHeight(mSize.y())
		.SetDepthOrArraySize(mSize.z())
		.SetName(name)
		.SetLayout(D3D12_TEXTURE_LAYOUT_ROW_MAJOR)
		.SetFlags(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
		.BuildDefault(device));

	mSrv = mResource->CreateSrv()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12Utils::GetSrvDimension(D3D12_RESOURCE_DIMENSION_BUFFER))
		.SetBuffer_FirstElement(0)
		.SetBuffer_NumElements(count)
		.SetBuffer_StructureByteStride(stride)
		.SetBuffer_Flags(D3D12_BUFFER_SRV_FLAG_NONE)
		.BuildBuffer();

	mUav = mResource->CreateUav()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12_UAV_DIMENSION_BUFFER)
		.SetBuffer_FirstElement(0)
		.SetBuffer_NumElements(count)
		.SetBuffer_StructureByteStride(stride)
		.SetBuffer_Flags(D3D12_BUFFER_UAV_FLAG_NONE)
		.BuildBuffer();
}

void D3D12RenderTarget::Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState)
{
	mResource->Transition(context, destState);
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

void D3D12RenderTarget::Clear(D3D12CommandContext* context, const Vec4f& color)
{
	Transition(context, D3D12_RESOURCE_STATE_RENDER_TARGET);

	using F4 = const FLOAT[4];
	context->GetCommandList()->ClearRenderTargetView(GetRtv()->GetHandle(), *(F4*)&color, 0, nullptr);
}
