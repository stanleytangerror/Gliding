#include "RenderPch.h"
#include "Texture.h"
#include "RenderTarget.h"

FileTexture::FileTexture(GI::IGraphicsInfra* infra, const char* filePath, const std::vector<b8>& content)
	: mFilePath(filePath)
	, mContent(content)
	, mTextureExtension(Utils::GetTextureExtension(filePath))
	, mImage(infra->CreateFromImageMemory(Utils::GetTextureExtension(filePath), mContent, filePath))
{
	
}

void FileTexture::CreateAndInitialResource(FrameGraph* frameGraph)
{
	mResource = frameGraph->CreatePermanent(
		mImage->GetResourceDesc(), 
		[this](GI::IGraphicsInfra* infra)
		{
			return std::move(infra->CreateMemoryResource(*mImage.get()));
		});
}

InMemoryTexture::InMemoryTexture(GI::IGraphicsInfra* infra, GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name)
	: mSize(size)
	, mContent(content)
	, mName(name)
	, mMipLevelCount(mipLevel)
	, mFormat(format)
{

}

void InMemoryTexture::CreateAndInitialResource(FrameGraph* frameGraph)
{
	mResource = frameGraph->CreatePermanent(
		GI::MemoryResourceDesc()
			.SetFormat(mFormat)
			.SetWidth(mSize.x())
			.SetHeight(mSize.y())
			.SetDepthOrArraySize(mSize.z())
			.SetMipLevels(mMipLevelCount)
			.SetName(mName.c_str()),
		[this](GI::IGraphicsInfra* infra)
		{
			return infra->CreateMemoryResourceFromTexture2DData(GI::ReadOnly2DResourceDesc()
				.SetData(mContent)
				.SetFormat(mFormat)
				.SetWidth(mSize.x())
				.SetHeight(mSize.y())
				.SetArraySize(mSize.z())
				.SetMipLevel(mMipLevelCount)
				.SetName(mName.c_str()));
		});
}