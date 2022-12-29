#pragma once
#pragma once

#include "D3D12Headers.h"
#include "D3D12Device.h"
#include "D3D12DescriptorAllocator.h"

namespace D3D12Backend
{
	class IResource;
}

class GD_D3D12BACKEND_API IRenderTargetView
{
public:
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE		GetHandle() const = 0;
	virtual DXGI_FORMAT							GetFormat() const = 0;
	virtual D3D12Backend::IResource*							GetResource() const = 0;
};

class GD_D3D12BACKEND_API IShaderResourceView
{
public:
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE		GetHandle() const = 0;
	virtual DXGI_FORMAT							GetFormat() const = 0;
	virtual D3D12Backend::IResource*							GetResource() = 0;
};

class GD_D3D12BACKEND_API SRV : public IShaderResourceView
{
public:
	SRV(D3D12Device* device, D3D12Backend::IResource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const override { return mDescriptionHandle.Get(); }
	DXGI_FORMAT									GetFormat() const override { return mFormat; }
	D3D12Backend::IResource*									GetResource() override { return mResource; }

protected:
	D3D12Backend::IResource*									mResource = nullptr;
	CpuDescItem									mDescriptionHandle;
	const DXGI_FORMAT							mFormat = DXGI_FORMAT_UNKNOWN;
};

class GD_D3D12BACKEND_API IUnorderedAccessView
{
public:
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE		GetHandle() const = 0;
	virtual DXGI_FORMAT							GetFormat() const = 0;
	virtual D3D12Backend::IResource*							GetResource() = 0;
};

class GD_D3D12BACKEND_API UAV : public IUnorderedAccessView
{
public:
	UAV(D3D12Device* device, D3D12Backend::IResource* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);

	CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const override { return mDescriptionHandle.Get(); }
	DXGI_FORMAT									GetFormat() const override { return mFormat; }
	D3D12Backend::IResource*									GetResource() override { return mResource; }
												
protected:										
	D3D12Backend::IResource*									mResource = nullptr;
	CpuDescItem									mDescriptionHandle;
	DXGI_FORMAT									mFormat = DXGI_FORMAT_UNKNOWN;
};

class GD_D3D12BACKEND_API RTV : public IRenderTargetView
{
public:
	RTV(D3D12Device* device, D3D12Backend::IResource* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc);

	CD3DX12_CPU_DESCRIPTOR_HANDLE	GetHandle() const override { return mRtv.Get(); }
	DXGI_FORMAT						GetFormat() const override { return mDesc.Format; }
	D3D12Backend::IResource*						GetResource() const override { return mResource; }
	void							Clear(D3D12CommandContext* context, const Vec4f& color);

protected:
	D3D12Backend::IResource* mResource = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC	mDesc;

	CpuDescItem				mRtv;
};

class GD_D3D12BACKEND_API DSV
{
public:
	DSV(D3D12Device* device, D3D12Backend::IResource* res, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);

	D3D12Backend::IResource*						GetResource() const { return mResource; }
	DXGI_FORMAT						GetFormat() const { return mFormat; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE	GetHandle() const { return mHandle.Get(); }

	void							Clear(D3D12CommandContext* context, float depth, const u32 stencil);

protected:
	D3D12Backend::IResource*						mResource = nullptr;
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