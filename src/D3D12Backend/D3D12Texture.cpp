#include "D3D12BackendPch.h"
#include "D3D12Texture.h"
#include "D3D12RenderTarget.h"

D3D12Texture::D3D12Texture(D3D12Device* device, const char* filePath, const std::vector<b8>& content)
	: mDevice(device)
	, mName(filePath)
	, mContent(content)
	, mFromImageMemory(true)
{

}

D3D12Texture::D3D12Texture(D3D12Device* device, DXGI_FORMAT format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name)
	: mDevice(device)
	, mName(name)
	, mSize(size)
	, mContent(content)
	, mMipLevelCount(mipLevel)
	, mFormat(format)
	, mFromImageMemory(false)
{

}

void D3D12Texture::Initial(D3D12CommandContext* context)
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
		.SetMipLevels(mResource->GetMipLevelCount())
		.BuildTex2D();
}

D3D12_RESOURCE_STATES D3D12Texture::GetResStates() const
{
	Assert(mResource != nullptr);

	return mResource->GetState();
}

