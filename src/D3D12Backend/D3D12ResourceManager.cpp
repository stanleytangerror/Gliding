#include "D3D12BackendPch.h"
#include "D3D12ResourceManager.h"
#include "D3D12Device.h"
#include "D3D12Resource.h"
#include "D3D12SwapChain.h"

namespace D3D12Backend
{
	ResourceManager::ResourceManager(D3D12Device* device)
		: mDevice(device)
	{
		for (i32 i = 0; i < mDescAllocator.size(); ++i)
		{
			mDescAllocator[i] = new D3D12DescriptorAllocator(mDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE(i));
		}
	}

	ResourceManager::~ResourceManager()
	{
		Assert(mReleaseQueue.empty());
	}

	std::unique_ptr<GI::IGraphicMemoryResource> ResourceManager::CreateResource(const GI::MemoryResourceDesc& desc)
	{
		auto resourceId = mResourceIdAllocator.Alloc();

		D3D12_RESOURCE_DESC d3d12Desc = {};
		{
			d3d12Desc.Dimension = D3D12_RESOURCE_DIMENSION(desc.GetDimension());
			d3d12Desc.Alignment = desc.GetAlignment();
			d3d12Desc.Width = desc.GetWidth();
			d3d12Desc.Height = desc.GetHeight();
			d3d12Desc.DepthOrArraySize = desc.GetDepthOrArraySize();
			d3d12Desc.MipLevels = desc.GetMipLevels();
			d3d12Desc.Format = D3D12Utils::ToDxgiFormat(desc.GetFormat());
			d3d12Desc.SampleDesc.Count = desc.GetSampleDesc_Count();
			d3d12Desc.SampleDesc.Quality = desc.GetSampleDesc_Quality();
			d3d12Desc.Layout = D3D12_TEXTURE_LAYOUT(desc.GetLayout());
			d3d12Desc.Flags = D3D12_RESOURCE_FLAGS(desc.GetFlags());
		}

		ID3D12Resource* resource = nullptr;
		CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE(desc.GetHeapType()));
		AssertHResultOk(mDevice->GetDevice()->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&d3d12Desc,
			D3D12_RESOURCE_STATES(desc.GetInitState()),
			nullptr,
			IID_PPV_ARGS(&resource)));
		NAME_RAW_D3D12_OBJECT(resource, desc.GetName());
		
		CommitedResource* result = new CommitedResource;
		result->mDevice = mDevice;
		result->mResource = resource;
		result->mSize = { (u32)desc.GetWidth(), (u32)desc.GetHeight(), desc.GetDepthOrArraySize() };
		result->mDesc = d3d12Desc;
		result->mState = D3D12_RESOURCE_STATES(desc.GetInitState());
		result->mHeapType = desc.GetHeapType();

		Assert(mResourceIdMapping.find(resourceId) == mResourceIdMapping.end());
		mResourceIdMapping[resourceId] = std::unique_ptr<CommitedResource>(result);

		return std::unique_ptr<GI::IGraphicMemoryResource>(new GraphicMemoryResource(mDevice, resourceId));
	}

	std::unique_ptr<GI::IGraphicMemoryResource> ResourceManager::CreateResource(ID3D12Resource* resource, const char* name, D3D12_RESOURCE_STATES currentState)
	{
		auto resourceId = mResourceIdAllocator.Alloc();

		NAME_RAW_D3D12_OBJECT(resource, name);

		CommitedResource* result = new CommitedResource;
		const D3D12_RESOURCE_DESC& desc = resource->GetDesc();
		result->mDevice = mDevice;
		result->mResource = resource;
		result->mSize = { u32(desc.Width), u32(desc.Height), desc.DepthOrArraySize };
		result->mDesc = desc;
		result->mState = currentState;

		Assert(mResourceIdMapping.find(resourceId) == mResourceIdMapping.end());
		mResourceIdMapping[resourceId] = std::unique_ptr<CommitedResource>(result);

		return std::unique_ptr<GI::IGraphicMemoryResource>(new GraphicMemoryResource(mDevice, resourceId));
	}


	std::unique_ptr<GI::IGraphicMemoryResource> ResourceManager::CreateResource(const D3D12_RESOURCE_DESC& desc, D3D12_HEAP_TYPE heapType, const char* name, D3D12_RESOURCE_STATES currentState)
	{
		auto resourceId = mResourceIdAllocator.Alloc();

		ID3D12Resource* resource = nullptr;
		CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
		AssertHResultOk(mDevice->GetDevice()->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			currentState,
			nullptr,
			IID_PPV_ARGS(&resource)));
		NAME_RAW_D3D12_OBJECT(resource, name);

		CommitedResource* result = new CommitedResource;
		result->mDevice = mDevice;
		result->mResource = resource;
		result->mSize = { u32(desc.Width), desc.Height, desc.DepthOrArraySize };
		result->mDesc = desc;
		result->mState = currentState;
		result->mHeapType = GI::HeapType::Enum(heapType);

		Assert(mResourceIdMapping.find(resourceId) == mResourceIdMapping.end());
		mResourceIdMapping[resourceId] = std::unique_ptr<CommitedResource>(result);

		return std::unique_ptr<GI::IGraphicMemoryResource>(new GraphicMemoryResource(mDevice, resourceId));
	}

	DescriptorPtr ResourceManager::CreateSrvDescriptor(GI::CommittedResourceId resourceId, const GI::SrvDesc& desc)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC d3d12Desc = {};
		{
			d3d12Desc.Format = D3D12Utils::ToDxgiFormat(desc.GetFormat());
			d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION(desc.GetViewDimension());
			const auto& mapping = desc.GetShader4ComponentMapping();
			d3d12Desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(mapping[0], mapping[1], mapping[2], mapping[3]);
			switch (desc.GetViewDimension())
			{
			case GI::SrvDimension::BUFFER:
				d3d12Desc.Buffer.FirstElement = desc.GetBuffer_FirstElement();
				d3d12Desc.Buffer.NumElements = desc.GetBuffer_NumElements();
				d3d12Desc.Buffer.StructureByteStride = desc.GetBuffer_StructureByteStride();
				d3d12Desc.Buffer.Flags = desc.GetBuffer_FlagRawRatherThanNone() ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
				break;
			case GI::SrvDimension::TEXTURE2D:
				d3d12Desc.Texture2D.MostDetailedMip = desc.GetTexture2D_MostDetailedMip();
				d3d12Desc.Texture2D.MipLevels = desc.GetTexture2D_MipLevels();
				d3d12Desc.Texture2D.PlaneSlice = desc.GetTexture2D_PlaneSlice();
				d3d12Desc.Texture2D.ResourceMinLODClamp = desc.GetTexture2D_ResourceMinLODClamp();
				break;
			}
		}

		auto res = resourceId ? GetResource(resourceId)->GetD3D12Resource() : nullptr;
		const auto& ptr = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->AllocCpuDesc();
		mDevice->GetDevice()->CreateShaderResourceView(res, &d3d12Desc, ptr.Get());
		return { ptr };
	}

	DescriptorPtr ResourceManager::CreateUavDescriptor(GI::CommittedResourceId resourceId, const GI::UavDesc& desc)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC d3d12Desc = {};
		{
			d3d12Desc.Format = D3D12Utils::ToDxgiFormat(desc.GetFormat());
			d3d12Desc.ViewDimension = D3D12_UAV_DIMENSION(desc.GetViewDimension());
			switch (desc.GetViewDimension())
			{
			case GI::UavDimension::BUFFER:
				d3d12Desc.Buffer.FirstElement = desc.GetBuffer_FirstElement();
				d3d12Desc.Buffer.NumElements = desc.GetBuffer_NumElements();
				d3d12Desc.Buffer.StructureByteStride = desc.GetBuffer_StructureByteStride();
				d3d12Desc.Buffer.CounterOffsetInBytes = desc.GetBuffer_CounterOffsetInBytes();
				d3d12Desc.Buffer.Flags = desc.GetBuffer_FlagRawRatherThanNone() ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
				break;
			case GI::UavDimension::TEXTURE2D:
				d3d12Desc.Texture2D.MipSlice = desc.GetTexture2D_MipSlice();
				d3d12Desc.Texture2D.PlaneSlice = desc.GetTexture2D_PlaneSlice();
				break;
			}
		}

		auto res = resourceId ? GetResource(resourceId)->GetD3D12Resource() : nullptr;
		const auto& ptr = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->AllocCpuDesc();
		mDevice->GetDevice()->CreateUnorderedAccessView(res, nullptr, &d3d12Desc, ptr.Get());
		return { ptr };
	}

	DescriptorPtr ResourceManager::CreateRtvDescriptor(GI::CommittedResourceId resourceId, const GI::RtvDesc& desc)
	{
		D3D12_RENDER_TARGET_VIEW_DESC d3d12Desc = {};
		{
			d3d12Desc.Format = D3D12Utils::ToDxgiFormat(desc.GetFormat());
			d3d12Desc.ViewDimension = D3D12_RTV_DIMENSION(desc.GetViewDimension());
			switch (desc.GetViewDimension())
			{
			case GI::RtvDimension::TEXTURE2D:
				d3d12Desc.Texture2D.MipSlice = desc.GetTexture2D_MipSlice();
				d3d12Desc.Texture2D.PlaneSlice = desc.GetTexture2D_PlaneSlice();
				break;
			}
		}

		auto res = resourceId ? GetResource(resourceId)->GetD3D12Resource() : nullptr;
		const auto& ptr = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->AllocCpuDesc();
		mDevice->GetDevice()->CreateRenderTargetView(res, &d3d12Desc, ptr.Get());
		return { ptr };
	}

	DescriptorPtr ResourceManager::CreateDsvDescriptor(GI::CommittedResourceId resourceId, const GI::DsvDesc& desc)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC d3d12Desc = {};
		{
			d3d12Desc.Format = D3D12Utils::ToDxgiFormat(desc.GetFormat());
			d3d12Desc.ViewDimension = D3D12_DSV_DIMENSION(desc.GetViewDimension());
			d3d12Desc.Flags = D3D12_DSV_FLAGS(desc.GetFlags());
			switch (desc.GetViewDimension())
			{
			case GI::DsvDimension::TEXTURE2D:
				d3d12Desc.Texture2D.MipSlice = desc.GetTexture2D_MipSlice();
				break;
			}
		}

		auto res = resourceId ? GetResource(resourceId)->GetD3D12Resource() : nullptr;
		const auto& ptr = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->AllocCpuDesc();
		mDevice->GetDevice()->CreateDepthStencilView(res, &d3d12Desc, ptr.Get());
		return { ptr };
	}

	DescriptorPtr ResourceManager::CreateSampler(const GI::SamplerDesc& desc)
	{
		D3D12_SAMPLER_DESC d3d12Desc = {};
		{
			d3d12Desc.Filter = D3D12_FILTER(desc.GetFilter());
			d3d12Desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE(desc.GetAddress()[0]);
			d3d12Desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE(desc.GetAddress()[1]);
			d3d12Desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE(desc.GetAddress()[2]);
			d3d12Desc.MipLODBias = desc.GetMipLODBias();
			d3d12Desc.MaxAnisotropy = desc.GetMaxAnisotropy();
			d3d12Desc.ComparisonFunc = D3D12_COMPARISON_FUNC(desc.GetComparisonFunc());
			d3d12Desc.BorderColor[0] = desc.GetBorderColor().x();
			d3d12Desc.BorderColor[1] = desc.GetBorderColor().y();
			d3d12Desc.BorderColor[2] = desc.GetBorderColor().z();
			d3d12Desc.BorderColor[3] = desc.GetBorderColor().w();
			d3d12Desc.MinLOD = desc.GetMinLOD();
			d3d12Desc.MaxLOD = desc.GetMaxLOD();
		}

		const auto& ptr = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->AllocCpuDesc();
		mDevice->GetDevice()->CreateSampler(&d3d12Desc, ptr.Get());
		return { ptr };
	}

	void ResourceManager::ReleaseResource(GI::CommittedResourceId id)
	{
		ReleaseItem item;
		item.mResourceId = id;

		for (i32 t = 0; t < Count; ++t)
		{
			D3D12GpuQueue* q = mDevice->GetGpuQueue(D3D12GpuQueueType(t));
			item.mGpuQueueTimePoints[q] = q->GetGpuPlannedValue();
		};

		mReleaseQueue.push_back(item);
	}

	CommitedResource* ResourceManager::GetResource(GI::CommittedResourceId id) const
	{
		auto it = mResourceIdMapping.find(id);
		return it == mResourceIdMapping.end() ? nullptr : it->second.get();
	}

	void ResourceManager::Update()
	{
		for (auto it = mReleaseQueue.begin(); it != mReleaseQueue.end();)
		{
			const ReleaseItem& item = *it;

			if (std::all_of(item.mGpuQueueTimePoints.begin(), item.mGpuQueueTimePoints.end(),
				[](const auto& p) { return p.first->IsGpuValueFinished(p.second); }))
			{
				Assert(mResourceIdMapping.find(item.mResourceId) != mResourceIdMapping.end());
				mResourceIdMapping.erase(mResourceIdMapping.find(item.mResourceId));

				it = mReleaseQueue.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	D3D12Backend::SwapChain* ResourceManager::CreateSwapChain(u32 windowId, HWND windowHandle, const Vec2u& size, const int32_t frameCount)
	{
		Assert(mSwapChains.find(windowId) == mSwapChains.end());

		const auto& swapChain = new SwapChain(mDevice, windowHandle, size, frameCount);
		mSwapChains[windowId] = swapChain;
		return swapChain;
	}

	D3D12Backend::SwapChain* ResourceManager::GetSwapChain(u32 windowId) const
	{
		auto it = mSwapChains.find(windowId);
		return it->second;
	}

}