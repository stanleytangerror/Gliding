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

void RTV::Clear(ID3D12GraphicsCommandList* commandList, const FLOAT color[4])
{
	GetResource()->Transition(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(GetHandle(), color, 0, nullptr);
}