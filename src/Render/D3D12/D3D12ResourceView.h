#pragma once
#pragma once

#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12DescriptorAllocator.h"

class IResource;

class IRenderTargetView
{
public:
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE		GetHandle() const = 0;
	virtual DXGI_FORMAT							GetFormat() const = 0;
	virtual IResource*							GetResource() const = 0;
};

class IShaderResourceView
{
public:
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE		GetHandle() const = 0;
	virtual DXGI_FORMAT							GetFormat() const = 0;
	virtual IResource*							GetResource() = 0;
};

class SRV : public IShaderResourceView
{
public:
	SRV(D3D12Device* device, IResource* res);
	SRV(D3D12Device* device, IResource* res, DXGI_FORMAT format);
	SRV(D3D12Device* device, IResource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

	CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const override { return mDescriptionHandle.Get(); }
	DXGI_FORMAT									GetFormat() const override { return mFormat; }
	IResource*									GetResource() override { return mResource; }

protected:
	IResource*									mResource = nullptr;
	CpuDescItem									mDescriptionHandle;
	const DXGI_FORMAT							mFormat = DXGI_FORMAT_UNKNOWN;
};

class IUnorderedAccessView
{
public:
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE		GetHandle() const = 0;
	virtual DXGI_FORMAT							GetFormat() const = 0;
	virtual IResource*							GetResource() = 0;
};

class UAV : public IUnorderedAccessView
{
public:
	UAV(D3D12Device* device, IResource* res);

	CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const override { return mDescriptionHandle.Get(); }
	DXGI_FORMAT									GetFormat() const override { return mFormat; }
	IResource*									GetResource() override { return mResource; }
												
protected:										
	IResource*									mResource = nullptr;
	CpuDescItem									mDescriptionHandle;
	DXGI_FORMAT									mFormat = DXGI_FORMAT_UNKNOWN;
};
