#pragma once

#include "Common/Texture.h"
#include "Common/GraphicsInfrastructure.h"

//class GD_RENDER_API Texture
//{
//public:
//	Texture(const char* filePath, const std::vector<b8>& content);
//	Texture(GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name);
//
//	void							Initial(D3D12Backend::D3D12CommandContext* context);
//
//	Vec3i							GetSize() const { return mSize; }
//	GI::Format::Enum				GetFormat() const { return mFormat; }
//	const GI::SrvDesc&							GetSrv() const { return mSrv; }
//
//	bool							IsD3DResourceReady() const { return mResource != nullptr; }
//
//protected:
//	std::vector<b8>			mContent;
//	bool					mFromImageMemory = true;
//
//	std::string				mFilePath;
//	std::string				mName;
//	std::unique_ptr<D3D12Backend::CommitedResource>			mResource;
//	Vec3i					mSize = {};
//	i32						mMipLevelCount = 1;
//	GI::Format::Enum		mFormat = GI::Format::FORMAT_UNKNOWN;
//
//	const GI::SrvDesc&					mSrv = nullptr;
//};

class GD_RENDER_API FileTexture
{
public:
	FileTexture(GI::IGraphicsInfra* infra, const char* filePath, const std::vector<b8>& content);

	void CreateAndInitialResource(GI::IGraphicsInfra* infra);
	bool							IsD3DResourceReady() const { return mResource != nullptr; }
	GI::IGraphicMemoryResource*		GetResource() const { return mResource.get(); }
	GI::SrvDesc						GetSrv() const;

protected:
	std::vector<b8>	const			mContent;
	std::string const				mFilePath;
	TextureFileExt::Enum const		mTextureExtension;

	std::unique_ptr<GI::IImage>		mImage;

	std::unique_ptr<GI::IGraphicMemoryResource>	mResource;
};

class GD_RENDER_API InMemoryTexture
{
public:
	InMemoryTexture(GI::IGraphicsInfra* infra, GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name);

	void CreateAndInitialResource(GI::IGraphicsInfra* infra);
	bool							IsD3DResourceReady() const { return mResource != nullptr; }
	GI::IGraphicMemoryResource*		GetResource() const { return mResource.get(); }

protected:
	std::vector<b8>	const	mContent;
	std::string	const		mName;
	Vec3i const				mSize = {};
	i32 const				mMipLevelCount = 1;
	GI::Format::Enum const	mFormat = GI::Format::FORMAT_UNKNOWN;
	
	std::unique_ptr<GI::IImage>		mImage;

	std::unique_ptr<GI::IGraphicMemoryResource>	mResource;
};
