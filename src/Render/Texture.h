#pragma once

#include "RenderInterface/RenderResource.h"
#include "D3D12Backend/D3D12Headers.h"
#include "D3D12Backend/D3D12Resource.h"
#include "D3D12Backend/D3D12ResourceView.h"
#include "D3D12Backend/D3D12CommandContext.h"
#include "Common/Texture.h"
#include <memory>

class GD_RENDER_API Texture
{
public:
	Texture(const char* filePath, const std::vector<b8>& content);
	Texture(DXGI_FORMAT format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name);

	void							Initial(D3D12Backend::D3D12CommandContext* context);

	Vec3i							GetSize() const { return mSize; }
	DXGI_FORMAT						GetFormat() const { return mFormat; }
	D3D12Backend::ShaderResourceView*							GetSrv() const { return mSrv; }

	bool							IsD3DResourceReady() const { return mResource != nullptr; }

protected:
	std::vector<b8>			mContent;
	bool					mFromImageMemory = true;

	D3D12Backend::D3D12Device* const		mDevice = nullptr;
	std::string				mFilePath;
	std::string				mName;
	std::unique_ptr<D3D12Backend::CommitedResource>			mResource;
	Vec3i					mSize = {};
	i32						mMipLevelCount = 1;
	DXGI_FORMAT				mFormat = DXGI_FORMAT_UNKNOWN;

	D3D12Backend::ShaderResourceView*					mSrv = nullptr;
};

class TextureFromFileInitializer : public RenderResourceInitializer
{
public:
	TextureFromFileInitializer(const char* name, TextureFileExt::Enum ext, const std::vector<b8>& content) : mName(name), mExt(ext), mContent(content) {}

	std::unique_ptr<RHI::ResourceObject> Initialize(D3D12Backend::D3D12CommandContext* context) override;

	std::string mName;
	TextureFileExt::Enum mExt;
	std::vector<b8> mContent;
};