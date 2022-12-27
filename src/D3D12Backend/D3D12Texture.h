#pragma once

#include "D3D12Headers.h"
#include "D3D12Resource.h"
#include "D3D12ResourceView.h"
#include "D3D12CommandContext.h"
#include "Common/Texture.h"

namespace D3D12Backend
{
	class CommitedResource;
}

class GD_D3D12BACKEND_API D3D12Texture : public ID3D12Res
{
public:
	D3D12Texture(D3D12Device* device, const char* filePath, const std::vector<b8>& content);
	D3D12Texture(D3D12Device* device, DXGI_FORMAT format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name);

	void							Initial(D3D12CommandContext* context);

	ID3D12Resource*					GetD3D12Resource() const override;
	Vec3i							GetSize() const override { return mSize; }
	DXGI_FORMAT						GetFormat() const { return mFormat; }

	bool							IsD3DResourceReady() const { return mResource != nullptr; }

	D3D12_RESOURCE_STATES			GetResStates() const { return mResStates; }
	SRV*							GetSrv() const { return mSrv; }

	void							Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) override;

protected:
	std::vector<b8>			mContent;
	bool					mFromImageMemory = true;

	D3D12Device* const		mDevice = nullptr;
	std::string				mFilePath;
	std::string				mName;
	std::unique_ptr<D3D12Backend::CommitedResource>			mResource;
	D3D12_RESOURCE_STATES	mResStates = D3D12_RESOURCE_STATE_COPY_DEST;
	Vec3i					mSize = {};
	i32						mMipLevelCount = 1;
	DXGI_FORMAT				mFormat = DXGI_FORMAT_UNKNOWN;

	SRV*					mSrv = nullptr;
};
