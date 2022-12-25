#pragma once
#pragma once

#include "D3D12Headers.h"
#include "D3D12Device.h"
#include "D3D12DescriptorAllocator.h"

class ID3D12Res;

class GD_D3D12BACKEND_API IRenderTargetView
{
public:
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE		GetHandle() const = 0;
	virtual DXGI_FORMAT							GetFormat() const = 0;
	virtual ID3D12Res*							GetResource() const = 0;
};

class GD_D3D12BACKEND_API IShaderResourceView
{
public:
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE		GetHandle() const = 0;
	virtual DXGI_FORMAT							GetFormat() const = 0;
	virtual ID3D12Res*							GetResource() = 0;
};

class GD_D3D12BACKEND_API SRV : public IShaderResourceView
{
public:
	SRV(D3D12Device* device, ID3D12Res* res);
	SRV(D3D12Device* device, ID3D12Res* res, DXGI_FORMAT format);
	SRV(D3D12Device* device, ID3D12Res* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const override { return mDescriptionHandle.Get(); }
	DXGI_FORMAT									GetFormat() const override { return mFormat; }
	ID3D12Res*									GetResource() override { return mResource; }

protected:
	ID3D12Res*									mResource = nullptr;
	CpuDescItem									mDescriptionHandle;
	const DXGI_FORMAT							mFormat = DXGI_FORMAT_UNKNOWN;
};

class GD_D3D12BACKEND_API IUnorderedAccessView
{
public:
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE		GetHandle() const = 0;
	virtual DXGI_FORMAT							GetFormat() const = 0;
	virtual ID3D12Res*							GetResource() = 0;
};

class GD_D3D12BACKEND_API UAV : public IUnorderedAccessView
{
public:
	UAV(D3D12Device* device, ID3D12Res* res);
	UAV(D3D12Device* device, ID3D12Res* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);

	CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const override { return mDescriptionHandle.Get(); }
	DXGI_FORMAT									GetFormat() const override { return mFormat; }
	ID3D12Res*									GetResource() override { return mResource; }
												
protected:										
	ID3D12Res*									mResource = nullptr;
	CpuDescItem									mDescriptionHandle;
	DXGI_FORMAT									mFormat = DXGI_FORMAT_UNKNOWN;
};

class GD_D3D12BACKEND_API RTV : public IRenderTargetView
{
public:
	RTV(D3D12Device* device, ID3D12Res* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc);

	CD3DX12_CPU_DESCRIPTOR_HANDLE	GetHandle() const override { return mRtv.Get(); }
	DXGI_FORMAT						GetFormat() const override { return mDesc.Format; }
	ID3D12Res*						GetResource() const override { return mResource; }
	void							Clear(D3D12CommandContext* context, const Vec4f& color);

protected:
	ID3D12Res* mResource = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC	mDesc;

	CpuDescItem				mRtv;
};

class GD_D3D12BACKEND_API DSV
{
public:
	DSV(D3D12Device* device, ID3D12Res* res, DXGI_FORMAT format);
	DSV(D3D12Device* device, ID3D12Res* res, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);

	ID3D12Res*						GetResource() const { return mResource; }
	DXGI_FORMAT						GetFormat() const { return mFormat; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE	GetHandle() const { return mHandle.Get(); }

	void							Clear(D3D12CommandContext* context, float depth, const u32 stencil);

protected:
	ID3D12Res*						mResource = nullptr;
	CpuDescItem						mHandle;
	DXGI_FORMAT						mFormat = DXGI_FORMAT_UNKNOWN;
};

class GD_D3D12BACKEND_API D3D12SamplerView
{
public:
	D3D12SamplerView(D3D12Device* device, D3D12_FILTER filterType, const std::array< D3D12_TEXTURE_ADDRESS_MODE, 3>& addrMode);
	D3D12SamplerView(D3D12Device* device, const D3D12_SAMPLER_DESC& desc);
	CD3DX12_CPU_DESCRIPTOR_HANDLE	GetHandle() const { return mHandle.Get(); }
	
protected:
	static D3D12_SAMPLER_DESC		GetDesc(D3D12_FILTER filterType, const std::array< D3D12_TEXTURE_ADDRESS_MODE, 3>& addrMode);

	D3D12_SAMPLER_DESC				mDesc = {};
	CpuDescItem						mHandle;
};