#include "RenderPch.h"
#include "Texture.h"
#include "RenderTarget.h"

Texture::Texture(const char* filePath, const std::vector<b8>& content)
	: mName(filePath)
	, mContent(content)
	, mFromImageMemory(true)
{

}

Texture::Texture(DXGI_FORMAT format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name)
	: mName(name)
	, mSize(size)
	, mContent(content)
	, mMipLevelCount(mipLevel)
	, mFormat(format)
	, mFromImageMemory(false)
{

}

void Texture::Initial(D3D12Backend::D3D12CommandContext* context)
{
	if (mFromImageMemory)
	{
		const TextureFileExt::Enum ext = Utils::GetTextureExtension(mName.c_str());
		mResource = D3D12Utils::CreateTextureFromImageMemory(context, ext, mContent);

		mFormat = mResource->GetFormat();
		mMipLevelCount = mResource->GetMipLevelCount();
		mSize = mResource->GetSize();
	}
	else
	{
		mResource = D3D12Utils::CreateTextureFromRawMemory(context, mFormat, mContent, mSize, mMipLevelCount, mName.c_str());

		Assert(mFormat == mResource->GetFormat());
		Assert(mMipLevelCount == mResource->GetMipLevelCount());
		Assert(mSize == mResource->GetSize());
	}

	NAME_RAW_D3D12_OBJECT(mResource->GetD3D12Resource(), mFilePath.c_str());

	mSrv = mResource->CreateSrv()
		.SetFormat(mFormat)
		.SetViewDimension(D3D12_SRV_DIMENSION_TEXTURE2D)
		.SetTexture2D(D3D12_TEX2D_SRV{ 0, mResource->GetMipLevelCount(), 0, 0 })
		.Build();
}

std::unique_ptr<RHI::ResourceObject> TextureFromFileInitializer::Initialize(D3D12Backend::D3D12CommandContext* context)
{
	auto resource = D3D12Utils::CreateTextureFromImageMemory(context, mExt, mContent);
	NAME_RAW_D3D12_OBJECT(resource->GetD3D12Resource(), mName.c_str());

	return resource;
}
