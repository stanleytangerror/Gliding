#pragma once

#include "Common/Texture.h"
#include "Common/GraphicsInfrastructure.h"
#include "FrameGraph.h"

class GD_RENDER_API FileTexture
{
public:
	FileTexture(FrameGraph* frameGraph, const char* filePath, const std::vector<b8>& content);

	FrameGraphResource				GetResource() const { return mResource; }

protected:
	std::vector<b8>	const			mContent;
	std::string const				mFilePath;
	TextureFileExt::Enum const		mTextureExtension;

	std::unique_ptr<GI::IImage>		mImage;

	FrameGraphResource				mResource;
};

class GD_RENDER_API InMemoryTexture
{
public:
	InMemoryTexture(FrameGraph* frameGraph, GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name);

	FrameGraphResource				GetResource() const { return mResource; }

protected:
	std::vector<b8>	const	mContent;
	std::string	const		mName;
	Vec3i const				mSize = {};
	i32 const				mMipLevelCount = 1;
	GI::Format::Enum const	mFormat = GI::Format::FORMAT_UNKNOWN;
	
	FrameGraphResource		mResource;
};
