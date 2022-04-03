#include "RenderPch.h"
#include "D3D12ResourceView.h"
#include "D3D12Resource.h"

SRV::SRV(D3D12Device* device, ID3D12Res* res)
	: SRV(device, res, res->GetD3D12Resource()->GetDesc().Format)
{
}

SRV::SRV(D3D12Device* device, ID3D12Res* res, DXGI_FORMAT format)
	: mResource(res)
	, mFormat(format)
{
	const D3D12_RESOURCE_DESC& desc = mResource->GetD3D12Resource()->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;

	mDescriptionHandle = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->AllocCpuDesc();
	device->GetDevice()->CreateShaderResourceView(mResource->GetD3D12Resource(), &srvDesc, mDescriptionHandle.Get());
}

SRV::SRV(D3D12Device* device, ID3D12Res* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
	: mResource(res)
	, mFormat(desc.Format)
{
	mDescriptionHandle = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->AllocCpuDesc();
	device->GetDevice()->CreateShaderResourceView(mResource->GetD3D12Resource(), &desc, mDescriptionHandle.Get());
}

//////////////////////////////////////////////////////////////////////////

UAV::UAV(D3D12Device* device, ID3D12Res* res)
	: mResource(res)
{
	const D3D12_RESOURCE_DESC& resdesc = mResource->GetD3D12Resource()->GetDesc();

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	{
		desc.Format = resdesc.Format;
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
	}

	mDescriptionHandle = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->AllocCpuDesc();
	device->GetDevice()->CreateUnorderedAccessView(mResource->GetD3D12Resource(), nullptr, &desc, mDescriptionHandle.Get());
}

//////////////////////////////////////////////////////////////////////////

RTV::RTV(D3D12Device* device, ID3D12Res* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
	: mResource(res)
	, mDesc(desc)
{
	D3D12DescriptorAllocator* rtvDescAllocator = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	mRtv = rtvDescAllocator->AllocCpuDesc();
	device->GetDevice()->CreateRenderTargetView(res->GetD3D12Resource(), &mDesc, mRtv.Get());
}

void RTV::Clear(D3D12CommandContext* context, const FLOAT color[4])
{
	GetResource()->Transition(context, D3D12_RESOURCE_STATE_RENDER_TARGET);
	context->GetCommandList()->ClearRenderTargetView(GetHandle(), color, 0, nullptr);
}

//////////////////////////////////////////////////////////////////////////

DSV::DSV(D3D12Device* device, ID3D12Res* res, DXGI_FORMAT format)
	: mResource(res)
	, mFormat(format)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	{
		desc.Format = format;
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		desc.Flags = D3D12_DSV_FLAG_NONE;
		desc.Texture2D.MipSlice = 0;
	}

	D3D12DescriptorAllocator* rtvDescAllocator = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mHandle = rtvDescAllocator->AllocCpuDesc();
	device->GetDevice()->CreateDepthStencilView(mResource->GetD3D12Resource(), &desc, mHandle.Get());
}

//////////////////////////////////////////////////////////////////////////

D3D12SamplerView::D3D12SamplerView(D3D12Device* device, const D3D12_SAMPLER_DESC& desc)
	: mDesc(desc)
{
	D3D12DescriptorAllocator* samplerDescAllocator = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	mHandle = samplerDescAllocator->AllocCpuDesc();
	device->GetDevice()->CreateSampler(&mDesc, mHandle.Get());
}

D3D12SamplerView::D3D12SamplerView(D3D12Device* device, D3D12_FILTER filterType, const std::array< D3D12_TEXTURE_ADDRESS_MODE, 3>& addrMode)
	: mDesc(GetDesc(filterType, addrMode))
{
	D3D12DescriptorAllocator* samplerDescAllocator = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	mHandle = samplerDescAllocator->AllocCpuDesc();
	device->GetDevice()->CreateSampler(&mDesc, mHandle.Get());
}

D3D12_SAMPLER_DESC D3D12SamplerView::GetDesc(D3D12_FILTER filterType, const std::array< D3D12_TEXTURE_ADDRESS_MODE, 3>& addrMode)
{
	D3D12_SAMPLER_DESC desc = {};
	desc.Filter = filterType;
	desc.AddressU = addrMode[0];
	desc.AddressV = addrMode[1];
	desc.AddressW = addrMode[2];
	return desc;
}
