#include "RenderPch.h"
#include "Texture.h"
#include "RenderTarget.h"

FileTexture::FileTexture(GI::IGraphicsInfra* infra, const char* filePath, const std::vector<b8>& content)
	: mFilePath(filePath)
	, mContent(content)
	, mTextureExtension(Utils::GetTextureExtension(filePath))
	, mImage(infra->CreateFromImageMemory(Utils::GetTextureExtension(filePath), mContent))
{
	
}

void FileTexture::CreateAndInitialResource(GI::IGraphicsInfra* infra)
{
	auto res = infra->CreateMemoryResource(*mImage.get());
	std::swap(mResource, res);
}

GI::SrvUsage FileTexture::GetSrv() const
{
	auto result = GI::SrvUsage(mResource);
	result
		.SetFormat(mResource->GetFormat())
		.SetViewDimension(GI::SrvDimension::TEXTURE2D)
		.SetTexture2D_MipLevels(mResource->GetMipLevelCount());
	return result;
}

InMemoryTexture::InMemoryTexture(GI::IGraphicsInfra* infra, GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name)
	: mSize(size)
	, mContent(content)
	, mName(name)
	, mMipLevelCount(mipLevel)
	, mFormat(format)
	//, mImage(infra->CreateFromScratch(mFormat, content, size, mipLevel, name))
{

}

void InMemoryTexture::CreateAndInitialResource(GI::IGraphicsInfra* infra)
{
	mResource = infra->CreateMemoryResourceFromTexture2DData(GI::ReadOnly2DResourceDesc()
		.SetData(mContent)
		.SetFormat(mFormat)
		.SetWidth(mSize.x())
		.SetHeight(mSize.y())
		.SetArraySize(mSize.z())
		.SetMipLevel(mMipLevelCount)
		.SetName(mName.c_str()));
}