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
			mDescAllocator[i].reset(new D3D12DescriptorAllocator(mDevice->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE(i)));
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
		NAME_RAW_D3D12_OBJECT(resource, desc.GetName().c_str());

		CommitedResource* result = new CommitedResource;
		result->mDevice = mDevice;
		result->mResource = resource;
		result->mSize = { (u32)desc.GetWidth(), (u32)desc.GetHeight(), desc.GetDepthOrArraySize() };
		result->mDesc = d3d12Desc;
		result->mState = D3D12_RESOURCE_STATES(desc.GetInitState());
		result->mHeapType = desc.GetHeapType();

		Assert(mResourceIdMapping.find(resourceId) == mResourceIdMapping.end());
		mResourceIdMapping[resourceId] = std::unique_ptr<CommitedResource>(result);

		mMonitor.OnCreateResource(resource, d3d12Desc, desc.GetName().c_str());

		return std::unique_ptr<GI::IGraphicMemoryResource>(new GraphicMemoryResource(mDevice, resourceId, desc.GetName().c_str()));
	}

	std::unique_ptr<GI::IGraphicMemoryResource> ResourceManager::PossessResourceWithOwnership(ID3D12Resource* resource, const char* name, D3D12_RESOURCE_STATES currentState)
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

		mMonitor.OnPossessResourceWithOwnership(resource, desc, name);

		return std::unique_ptr<GI::IGraphicMemoryResource>(new GraphicMemoryResource(mDevice, resourceId, name));
	}

	DescriptorPtr ResourceManager::CreateSrvDescriptor(GI::CommittedResourceId resourceId, const GI::SrvDesc& desc)
	{
		if (mResourceViewMapping.find(resourceId) == mResourceViewMapping.end())
		{
			mResourceViewMapping[resourceId] = {};
		}

		auto& viewMapping = mResourceViewMapping[resourceId];

		const auto hash = Utils::HashPod(desc);
		auto it = viewMapping.find(hash);
		if (it != viewMapping.end())
		{
			return it->second.second;
		}

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
		auto descAlloc = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].get();
		const auto& ptr = descAlloc->AllocCpuDesc();
		mDevice->GetDevice()->CreateShaderResourceView(res, &d3d12Desc, ptr.Get());

		viewMapping[hash] = { descAlloc, ptr };
		return { ptr };
	}

	DescriptorPtr ResourceManager::CreateUavDescriptor(GI::CommittedResourceId resourceId, const GI::UavDesc& desc)
	{
		if (mResourceViewMapping.find(resourceId) == mResourceViewMapping.end())
		{
			mResourceViewMapping[resourceId] = {};
		}

		auto& viewMapping = mResourceViewMapping[resourceId];

		const auto hash = Utils::HashPod(desc);
		auto it = viewMapping.find(hash);
		if (it != viewMapping.end())
		{
			return it->second.second;
		}

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
		auto descAlloc = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].get();
		const auto& ptr = descAlloc->AllocCpuDesc();
		mDevice->GetDevice()->CreateUnorderedAccessView(res, nullptr, &d3d12Desc, ptr.Get());

		viewMapping[hash] = { descAlloc, ptr };
		return { ptr };
	}

	DescriptorPtr ResourceManager::CreateRtvDescriptor(GI::CommittedResourceId resourceId, const GI::RtvDesc& desc)
	{
		if (mResourceViewMapping.find(resourceId) == mResourceViewMapping.end())
		{
			mResourceViewMapping[resourceId] = {};
		}

		auto& viewMapping = mResourceViewMapping[resourceId];

		const auto hash = Utils::HashPod(desc);
		auto it = viewMapping.find(hash);
		if (it != viewMapping.end())
		{
			return it->second.second;
		}

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
		auto descAlloc = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].get();
		const auto& ptr = descAlloc->AllocCpuDesc();
		mDevice->GetDevice()->CreateRenderTargetView(res, &d3d12Desc, ptr.Get());

		viewMapping[hash] = { descAlloc, ptr };
		return { ptr };
	}

	DescriptorPtr ResourceManager::CreateDsvDescriptor(GI::CommittedResourceId resourceId, const GI::DsvDesc& desc)
	{
		if (mResourceViewMapping.find(resourceId) == mResourceViewMapping.end())
		{
			mResourceViewMapping[resourceId] = {};
		}

		auto& viewMapping = mResourceViewMapping[resourceId];

		const auto hash = Utils::HashPod(desc);
		auto it = viewMapping.find(hash);
		if (it != viewMapping.end())
		{
			return it->second.second;
		}

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
		auto descAlloc = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].get();
		const auto& ptr = descAlloc->AllocCpuDesc();
		mDevice->GetDevice()->CreateDepthStencilView(res, &d3d12Desc, ptr.Get());

		viewMapping[hash] = { descAlloc, ptr };
		return { ptr };
	}

	DescriptorPtr ResourceManager::CreateSampler(const GI::SamplerDesc& desc)
	{
		const auto hash = Utils::HashPod(desc);
		auto it = mSamplerMapping.find(hash);
		if (it != mSamplerMapping.end())
		{
			return it->second.second;
		}

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

		auto descAlloc = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].get();
		const auto& ptr = descAlloc->AllocCpuDesc();
		mDevice->GetDevice()->CreateSampler(&d3d12Desc, ptr.Get());

		mSamplerMapping[hash] = { descAlloc, ptr };
		return { ptr };
	}

	void ResourceManager::ReleaseResource(GI::CommittedResourceId id)
	{
		ReleaseItem item;
		item.mResourceId = id;

		// release view
		u64 plannedValue = 0;
		for (i32 t = 0; t < Count; ++t)
		{
			auto* q = mDevice->GetGpuQueue(D3D12GpuQueueType(t));
			plannedValue = std::max(plannedValue, q->GetGpuPlannedValue());
		};
		auto it = mResourceViewMapping.find(id);
		if (it != mResourceViewMapping.end())
		{
			for (auto& [_, p] : it->second)
			{
				p.first->ReleaseCpuDesc(plannedValue, p.second);
			}
		}
		
		// release resource
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
		u64 completedValue = std::numeric_limits<u64>::max();
		for (i32 t = 0; t < Count; ++t)
		{
			auto* q = mDevice->GetGpuQueue(D3D12GpuQueueType(t));
			completedValue = std::min(completedValue, q->GetGpuCompletedValue());
		};
		for (auto & allocator : mDescAllocator)
		{
			allocator->UpdateCompletedFenceValue(completedValue);
		}

		for (auto it = mReleaseQueue.begin(); it != mReleaseQueue.end();)
		{
			const ReleaseItem& item = *it;

			if (std::all_of(item.mGpuQueueTimePoints.begin(), item.mGpuQueueTimePoints.end(),
				[](const auto& p) { return p.first->IsGpuValueFinished(p.second); }))
			{
				auto deleteItem = mResourceIdMapping.find(item.mResourceId);
				Assert(deleteItem != mResourceIdMapping.end());
				mMonitor.OnReleaseResource(deleteItem->second->GetD3D12Resource());
				mResourceIdMapping.erase(deleteItem);

				it = mReleaseQueue.erase(it);
			}
			else
			{
				++it;
			}
		}

		mMonitor.PrintResourceStatistics();
	}

	void ResourceManager::ResourceMonitor::OnCreateResource(ID3D12Resource* resource, const D3D12_RESOURCE_DESC& desc, const char* name)
	{
		Assert(mResources.find(resource) == mResources.end());
		mResources[resource] = { name, desc, CalcMemorySize(desc) };
	}

	void ResourceManager::ResourceMonitor::OnPossessResourceWithOwnership(ID3D12Resource* resource, const D3D12_RESOURCE_DESC& desc, const char* name)
	{
		Assert(mResources.find(resource) == mResources.end());
		mResources[resource] = { name, desc, CalcMemorySize(desc) };
	}

	void ResourceManager::ResourceMonitor::OnReleaseResource(ID3D12Resource* resource)
	{
		Assert(mResources.find(resource) != mResources.end());
		mResources.erase(mResources.find(resource));
	}

	void ResourceManager::ResourceMonitor::PrintResourceStatistics()
	{
		auto printMemorySize = [](u32 size)
		{
			std::string str;
			if (size / 1000000000) { str += std::to_string(size / 1000000000) + ","; }
			if (size / 1000000) { str += std::to_string((size / 1000000) % 1000) + ","; }
			if (size / 1000) { str += std::to_string((size / 1000) % 1000) + ","; }
			str += std::to_string(size % 1000);
			return str;
		};

		i32 totalSize = 0;
		for (const auto& [_, status] : mResources)
		{
			totalSize += status.mMemorySize;
		}
		DEBUG_PRINT("Total resource size: %s", printMemorySize(totalSize).c_str());

		std::vector<DeviceResourceStatus> resources;
		for (const auto& [_, status] : mResources)
		{
			resources.push_back(status);
		}
		std::sort(resources.begin(), resources.end(), [](const auto& a, const auto& b) { return a.mMemorySize > b.mMemorySize; });
		for (const auto& status : resources)
		{
			DEBUG_PRINT("\t[Resource] memory size: %s \t dimension: (%d, %d, %d) \t name: %s", 
				printMemorySize(status.mMemorySize).c_str(), status.mDesc.Width, status.mDesc.Height, status.mDesc.DepthOrArraySize, status.mName.c_str());
		}
	}

	u32 ResourceManager::ResourceMonitor::CalcMemorySize(const D3D12_RESOURCE_DESC& desc)
	{
		switch (desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_BUFFER:
			return desc.Width;
		default:
			u32 bytes = DirectX::BitsPerPixel(desc.Format) >> 3;
			return desc.Width * desc.Height * desc.DepthOrArraySize * bytes;
		}
	}
}