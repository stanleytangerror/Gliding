#pragma once

#include "D3D12Resource.h"
#include "D3D12ResourceView.h"

class GD_D3D12BACKEND_API D3D12RenderTarget : public ID3D12Res
{
public:
	D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, const char* name);
	D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, i32 mipLevelCount, const char* name);
	D3D12RenderTarget(D3D12Device* device, i32 count, i32 stride, DXGI_FORMAT format, const char* name);

	void		Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) override;

	RTV* CreateTex2DRtv();
	RTV* CreateTex2DArrayRtv(uint32_t firstArrayIdx, uint32_t arrayCount);
	SRV* CreateTexCubeSrv();
	//DSV* CreateTex2DDsv();

	RTV* GetRtv() const { return mRtv; }
	SRV* GetSrv() const { return mSrv; }
	UAV* GetUav() const { return mUav; }

	DXGI_FORMAT							GetFormat() const { return mFormat; }
	D3D12_RESOURCE_STATES				GetResStates() const { return mState; }
	ID3D12Resource*						GetD3D12Resource() const override { return mResource->GetD3D12Resource(); }
	Vec3i								GetSize() const override { return mSize; }
	i32									GetMipLevelCount() const { return mMipLevelCount; }

	void								Clear(D3D12CommandContext* context, const Vec4f& color);

protected:
	D3D12Device*				mDevice = nullptr;
	std::unique_ptr<D3D12Backend::CommitedResource>	mResource;
	Vec3i						mSize = {};
	i32							mMipLevelCount = 1;
	DXGI_FORMAT					mFormat;
	D3D12_RESOURCE_STATES		mState = D3D12_RESOURCE_STATE_COMMON;

	RTV*						mRtv = nullptr;
	UAV*						mUav = nullptr;
	SRV*						mSrv = nullptr;
};
