#pragma once

#include "D3D12/D3D12Resource.h"
#include "D3D12/D3D12ResourceView.h"

class D3D12RenderTarget : public ID3D12Res
{
public:
	D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, const char* name);

	void		Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) override;

	RTV* CreateTex2DRtv();
	RTV* CreateTex2DArrayRtv(uint32_t firstArrayIdx, uint32_t arrayCount);
	SRV* CreateTexCubeSrv();
	//DSV* CreateTex2DDsv();

	RTV* GetRtv() const { return mRtv; }
	SRV* GetSrv() const { return mSrv; }
	UAV* GetHandle() const { return mUav; }

	DXGI_FORMAT							GetFormat() const { return mFormat; }
	D3D12_RESOURCE_STATES				GetResStates() const { return mState; }
	ID3D12Resource*						GetD3D12Resource() const override { return mResource; }
	Vec3i								GetSize() const override { return mSize; }

	void								Clear(D3D12CommandContext* context, const Vec4f& color);

protected:
	D3D12Device*				mDevice = nullptr;
	ID3D12Resource*				mResource = nullptr;
	Vec3i						mSize = {};
	DXGI_FORMAT					mFormat;
	D3D12_RESOURCE_STATES		mState = D3D12_RESOURCE_STATE_COMMON;

	RTV*						mRtv = nullptr;
	UAV*						mUav = nullptr;
	SRV*						mSrv = nullptr;
};


class D3DDepthStencil : public ID3D12Res
{
	// https://docs.microsoft.com/en-us/windows/desktop/direct3d11/d3d10-graphics-programming-guide-depth-stencil
public:
	D3DDepthStencil(D3D12Device* device, Vec2i size, DXGI_FORMAT format, DXGI_FORMAT dsvFormat, DXGI_FORMAT srvFormat, const char* name);

	void								Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState);
	DSV*								GetDsv() const { return mDsv; }
	SRV*								GetSrv() const { return mSrv; }
	DXGI_FORMAT							GetFormat() const { return mFormat; }
	D3D12_RESOURCE_STATES				GetResStates() const { return mState; }
	ID3D12Resource*						GetD3D12Resource() const override { return mResource; }
	Vec3i								GetSize() const override { return mSize; }

	void								Clear(D3D12CommandContext* context, float depth, UINT stencil);

protected:
	D3D12Device*				mDevice = nullptr;
	ID3D12Resource*				mResource = nullptr;
	Vec3i						mSize = {};
	SRV*						mSrv = nullptr;
	DSV*						mDsv = nullptr;
	DXGI_FORMAT					mFormat;
	D3D12_RESOURCE_STATES		mState = D3D12_RESOURCE_STATE_COMMON;
};
