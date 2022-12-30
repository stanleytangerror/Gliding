#pragma once
#pragma once

#include "D3D12Headers.h"
#include "D3D12DescriptorAllocator.h"
#include "D3D12Resource.h"

class D3D12Device;

namespace D3D12Backend
{
	class IResource;

	class GD_D3D12BACKEND_API ShaderResourceView
	{
	public:
		ShaderResourceView(D3D12Device* device, IResource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

		CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const { return mDescriptionHandle.Get(); }
		DXGI_FORMAT									GetFormat() const { return mDesc.Format; }
		IResource*									GetResource() { return mResource; }

	protected:
		IResource*									mResource = nullptr;
		CpuDescItem									mDescriptionHandle;
		D3D12_SHADER_RESOURCE_VIEW_DESC const		mDesc;
	};

	class GD_D3D12BACKEND_API UnorderedAccessView
	{
	public:
		UnorderedAccessView(D3D12Device* device, IResource* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);

		CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const { return mDescriptionHandle.Get(); }
		DXGI_FORMAT									GetFormat() const { return mDesc.Format; }
		IResource*									GetResource() { return mResource; }

	protected:
		IResource*									mResource = nullptr;
		CpuDescItem									mDescriptionHandle;
		D3D12_UNORDERED_ACCESS_VIEW_DESC const		mDesc;
	};

	class GD_D3D12BACKEND_API RenderTargetView
	{
	public:
		RenderTargetView(D3D12Device* device, IResource* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc);

		CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const { return mRtv.Get(); }
		DXGI_FORMAT									GetFormat() const { return mDesc.Format; }
		IResource*									GetResource() const { return mResource; }

		void										Clear(D3D12Backend::D3D12CommandContext* context, const Vec4f& color);

	protected:
		IResource*									mResource = nullptr;
		D3D12_RENDER_TARGET_VIEW_DESC const			mDesc;
		CpuDescItem									mRtv;
	};

	class GD_D3D12BACKEND_API DepthStencilView
	{
	public:
		DepthStencilView(D3D12Device* device, IResource* res, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);

		IResource*									GetResource() const { return mResource; }
		DXGI_FORMAT									GetFormat() const { return mDesc.Format; }
		CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const { return mHandle.Get(); }

		void										Clear(D3D12Backend::D3D12CommandContext* context, float depth, const u32 stencil);

	protected:
		IResource*									mResource = nullptr;
		CpuDescItem									mHandle;
		D3D12_DEPTH_STENCIL_VIEW_DESC const			mDesc;
	};

	class GD_D3D12BACKEND_API SamplerView
	{
	public:
		SamplerView(D3D12Device* device, D3D12_FILTER filterType, const std::array< D3D12_TEXTURE_ADDRESS_MODE, 3>& addrMode);
		SamplerView(D3D12Device* device, const D3D12_SAMPLER_DESC& desc);
		CD3DX12_CPU_DESCRIPTOR_HANDLE				GetHandle() const { return mHandle.Get(); }

	protected:
		static D3D12_SAMPLER_DESC					GetDesc(D3D12_FILTER filterType, const std::array< D3D12_TEXTURE_ADDRESS_MODE, 3>& addrMode);

		D3D12_SAMPLER_DESC const					mDesc = {};
		CpuDescItem									mHandle;
	};
}