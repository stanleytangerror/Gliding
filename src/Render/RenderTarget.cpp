#include "RenderPch.h"
#include "RenderTarget.h"
#include "D3D12Backend/D3D12Resource.h"
#include "D3D12Backend/D3D12Device.h"

RenderTarget::RenderTarget(D3D12Backend::D3D12Device* device, Vec3i size, DXGI_FORMAT format, const char* name)
	: RenderTarget(device, size, format, 1, name)
{
}

RenderTarget::RenderTarget(D3D12Backend::D3D12Device* device, Vec3i size, DXGI_FORMAT format, i32 mipLevelCount, const char* name)
	: mDevice(device)
	, mSize(size)
	, mMipLevelCount(mipLevelCount)
	, mFormat(format)
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
		.SetInitState(D3D12_RESOURCE_STATE_RENDER_TARGET)
		.BuildDefault(device));

	mSrv = mResource->CreateSrv()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12_SRV_DIMENSION_TEXTURE2D)
		.SetTexture2D(D3D12_TEX2D_SRV{ 0, (u32) mipLevelCount, 0, 0 })
		.Build();

	mRtv = mResource->CreateRtv()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12_RTV_DIMENSION_TEXTURE2D)
		.Build();

	mUav = mResource->CreateUav()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12_UAV_DIMENSION_TEXTURE2D)
		.BuildTex2D();
}

RenderTarget::RenderTarget(D3D12Backend::D3D12Device* device, i32 count, i32 stride, DXGI_FORMAT format, const char* name)
	: mDevice(device)
	, mSize(count * stride, 1, 1)
	, mFormat(format)
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
		.SetInitState(D3D12_RESOURCE_STATE_RENDER_TARGET)
		.BuildDefault(device));

	mSrv = mResource->CreateSrv()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12Utils::GetSrvDimension(D3D12_RESOURCE_DIMENSION_BUFFER))
		.SetBuffer(D3D12_BUFFER_SRV{ 0, (u32)count, (u32)stride, D3D12_BUFFER_SRV_FLAG_NONE })
		.Build();

	mUav = mResource->CreateUav()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12_UAV_DIMENSION_BUFFER)
		.SetBuffer_FirstElement(0)
		.SetBuffer_NumElements(count)
		.SetBuffer_StructureByteStride(stride)
		.SetBuffer_Flags(D3D12_BUFFER_UAV_FLAG_NONE)
		.BuildBuffer();
}