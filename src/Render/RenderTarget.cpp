#include "RenderPch.h"
#include "RenderTarget.h"

RenderTarget::RenderTarget(GI::IGraphicsInfra* infra, Vec3u size, GI::Format::Enum format, const char* name)
	: RenderTarget(infra, size, format, 1, name)
{
}

RenderTarget::RenderTarget(GI::IGraphicsInfra* infra, Vec3u size, GI::Format::Enum format, i32 mipLevelCount, const char* name)
	: mSize(size)
	, mMipLevelCount(mipLevelCount)
	, mFormat(format)
{
	mResource = infra->CreateMemoryResource(
		GI::MemoryResourceDesc()
		.SetAlignment(0)
		.SetDimension(GI::ResourceDimension::TEXTURE2D)
		.SetWidth(mSize.x())
		.SetHeight(mSize.y())
		.SetDepthOrArraySize(mSize.z())
		.SetMipLevels(mMipLevelCount)
		.SetFormat(mFormat)
		.SetLayout(GI::TextureLayout::LAYOUT_UNKNOWN)
		.SetFlags(GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS)
		.SetInitState(GI::ResourceState::STATE_RENDER_TARGET)
		.SetName(name)
		.SetHeapType(GI::HeapType::DEFAULT));

	mRtv = GI::RtvUsage(mResource);
	mRtv
		.SetFormat(mFormat)
		.SetViewDimension(GI::RtvDimension::TEXTURE2D);

	mUav = GI::UavUsage(mResource);
	mUav
		.SetFormat(mFormat)
		.SetViewDimension(GI::UavDimension::TEXTURE2D);

	mSrv = GI::SrvUsage(mResource);
	mSrv
		.SetFormat(mFormat)
		.SetViewDimension(GI::SrvDimension::TEXTURE2D)
		.SetTexture2D_MipLevels(u32(mMipLevelCount));
}

RenderTarget::RenderTarget(GI::IGraphicsInfra* infra, i32 count, i32 stride, GI::Format::Enum format, const char* name)
	: mSize(count * stride, 1, 1)
	, mFormat(format)
{
	mResource = infra->CreateMemoryResource(
		GI::MemoryResourceDesc()
		.SetAlignment(0)
		.SetDimension(GI::ResourceDimension::BUFFER)
		.SetWidth(mSize.x())
		.SetHeight(mSize.y())
		.SetDepthOrArraySize(mSize.z())
		.SetMipLevels(1)
		.SetFormat(mFormat)
		.SetLayout(GI::TextureLayout::LAYOUT_ROW_MAJOR)
		.SetFlags(GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS)
		.SetInitState(GI::ResourceState::STATE_RENDER_TARGET)
		.SetName(name)
		.SetHeapType(GI::HeapType::DEFAULT));

	mUav = GI::UavUsage(mResource);
	mUav
		.SetFormat(GI::Format::FORMAT_UNKNOWN)
		.SetViewDimension(GI::UavDimension::BUFFER)
		.SetBuffer_FirstElement(0)
		.SetBuffer_NumElements(count)
		.SetBuffer_StructureByteStride(u32(stride))
		.SetBuffer_FlagRawRatherThanNone(false);

	mSrv = GI::SrvUsage(mResource);
	mSrv
		.SetFormat(GI::Format::FORMAT_R32_UINT)
		.SetViewDimension(GI::SrvDimension::BUFFER)
		.SetBuffer_FirstElement(0)
		.SetBuffer_NumElements(count)
		.SetBuffer_FlagRawRatherThanNone(false);
}