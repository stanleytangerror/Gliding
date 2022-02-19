#pragma once

#include "Common/Math.h"

class IResource
{
public:
	virtual void Transition(ID3D12GraphicsCommandList* commandList, const D3D12_RESOURCE_STATES& destState) = 0;
	virtual ID3D12Resource* GetD3D12Resource() const = 0;
	virtual Vec3i GetSize() const = 0;
};


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
	SRV(IResource* res);
	SRV(IResource* res, DXGI_FORMAT format);
	SRV(IResource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

	CD3DX12_CPU_DESCRIPTOR_HANDLE	GetHandle() const override { return mDescriptionHandle.mHandle; }
	DXGI_FORMAT					GetFormat() const override { return mFormat; }
	IResource*					GetResource() override { return mResource; }

protected:
	IResource*					mResource = nullptr;
	SCpuDescriptorItem			mDescriptionHandle;
	const DXGI_FORMAT			mFormat = DXGI_FORMAT_UNKNOWN;
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
	UAV(IResource* res);

	CD3DX12_CPU_DESCRIPTOR_HANDLE	GetHandle() const override { return mDescriptionHandle.mHandle; }
	DXGI_FORMAT					GetFormat() const override { return mFormat; }
	IResource*					GetResource() override { return mResource; }

protected:
	IResource*					mResource = nullptr;
	SCpuDescriptorItem			mDescriptionHandle;
	DXGI_FORMAT					mFormat = DXGI_FORMAT_UNKNOWN;
};
