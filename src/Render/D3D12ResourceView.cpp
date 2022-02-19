#include "D3D12ResourceView.h"
#include "D3D12Resource.h"

SRV::SRV(D3D12Device* device, IResource* res)
	: SRV(device, res, res->GetD3D12Resource()->GetDesc().Format)
{
}

SRV::SRV(D3D12Device* device, IResource* res, DXGI_FORMAT format)
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

SRV::SRV(D3D12Device* device, IResource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
	: mResource(res)
	, mFormat(desc.Format)
{
	mDescriptionHandle = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->AllocCpuDesc();
	device->GetDevice()->CreateShaderResourceView(mResource->GetD3D12Resource(), &desc, mDescriptionHandle.Get());
}

UAV::UAV(D3D12Device* device, IResource* res)
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
