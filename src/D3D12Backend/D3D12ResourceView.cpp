#include "D3D12BackendPch.h"
#include "D3D12ResourceView.h"
#include "D3D12Resource.h"
#include "D3D12Device.h"

namespace D3D12Backend
{
	ShaderResourceView::ShaderResourceView(D3D12Device* device, IResource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
		: mResource(res)
		, mDesc(desc)
	{
		mDescriptionHandle = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->AllocCpuDesc();
		device->GetDevice()->CreateShaderResourceView(mResource->GetD3D12Resource(), &desc, mDescriptionHandle.Get());
	}

	//////////////////////////////////////////////////////////////////////////

	UnorderedAccessView::UnorderedAccessView(D3D12Device* device, IResource* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
		: mResource(res)
		, mDesc(desc)
	{
		mDescriptionHandle = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->AllocCpuDesc();
		device->GetDevice()->CreateUnorderedAccessView(mResource->GetD3D12Resource(), nullptr, &desc, mDescriptionHandle.Get());
	}

	//////////////////////////////////////////////////////////////////////////

	RenderTargetView::RenderTargetView(D3D12Device* device, IResource* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
		: mResource(res)
		, mDesc(desc)
	{
		D3D12DescriptorAllocator* rtvDescAllocator = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		mRtv = rtvDescAllocator->AllocCpuDesc();
		device->GetDevice()->CreateRenderTargetView(res->GetD3D12Resource(), &mDesc, mRtv.Get());
	}

	void RenderTargetView::Clear(D3D12Backend::D3D12CommandContext* context, const Vec4f& color)
	{
		GetResource()->Transition(context, D3D12_RESOURCE_STATE_RENDER_TARGET);

		using F4 = const FLOAT[4];
		context->GetCommandList()->ClearRenderTargetView(GetHandle(), *(F4*)&color, 0, nullptr);
	}

	//////////////////////////////////////////////////////////////////////////

	DepthStencilView::DepthStencilView(D3D12Device* device, IResource* res, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
		: mResource(res)
		, mDesc(desc)
	{
		D3D12DescriptorAllocator* rtvDescAllocator = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		mHandle = rtvDescAllocator->AllocCpuDesc();
		device->GetDevice()->CreateDepthStencilView(mResource->GetD3D12Resource(), &desc, mHandle.Get());
	}

	void DepthStencilView::Clear(D3D12Backend::D3D12CommandContext* context, float depth, const u32 stencil)
	{
		GetResource()->Transition(context, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		context->GetCommandList()->ClearDepthStencilView(GetHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
	}

	//////////////////////////////////////////////////////////////////////////

	SamplerView::SamplerView(D3D12Device* device, const D3D12_SAMPLER_DESC& desc)
		: mDesc(desc)
	{
		D3D12DescriptorAllocator* samplerDescAllocator = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		mHandle = samplerDescAllocator->AllocCpuDesc();
		device->GetDevice()->CreateSampler(&mDesc, mHandle.Get());
	}

	SamplerView::SamplerView(D3D12Device* device, D3D12_FILTER filterType, const std::array< D3D12_TEXTURE_ADDRESS_MODE, 3>& addrMode)
		: mDesc(GetDesc(filterType, addrMode))
	{
		D3D12DescriptorAllocator* samplerDescAllocator = device->GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		mHandle = samplerDescAllocator->AllocCpuDesc();
		device->GetDevice()->CreateSampler(&mDesc, mHandle.Get());
	}

	D3D12_SAMPLER_DESC SamplerView::GetDesc(D3D12_FILTER filterType, const std::array< D3D12_TEXTURE_ADDRESS_MODE, 3>& addrMode)
	{
		D3D12_SAMPLER_DESC desc = {};
		desc.Filter = filterType;
		desc.AddressU = addrMode[0];
		desc.AddressV = addrMode[1];
		desc.AddressW = addrMode[2];
		desc.MinLOD = 0;
		desc.MaxLOD = std::numeric_limits<f32>::max();
		return desc;
	}
}