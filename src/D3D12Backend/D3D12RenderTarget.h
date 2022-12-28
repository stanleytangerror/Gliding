#pragma once

#include "D3D12Resource.h"
#include "D3D12ResourceView.h"

class GD_D3D12BACKEND_API D3D12RenderTarget
{
public:
	D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, const char* name);
	D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, i32 mipLevelCount, const char* name);
	D3D12RenderTarget(D3D12Device* device, i32 count, i32 stride, DXGI_FORMAT format, const char* name);

	RTV*						GetRtv() const { return mRtv; }
	SRV*						GetSrv() const { return mSrv; }
	UAV*						GetUav() const { return mUav; }

	void						Clear(D3D12CommandContext* context, const Vec4f& color);

protected:
	D3D12Device*				mDevice = nullptr;
	std::unique_ptr<D3D12Backend::CommitedResource>	
								mResource;
	Vec3i						mSize = {};
	i32							mMipLevelCount = 1;
	DXGI_FORMAT					mFormat;

	RTV*						mRtv = nullptr;
	UAV*						mUav = nullptr;
	SRV*						mSrv = nullptr;
};
