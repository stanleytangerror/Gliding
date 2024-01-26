#pragma once

#include "D3D12Headers.h"
#include "D3D12DescriptorAllocator.h"
#include "Common/IndexAllocator.h"

namespace D3D12Backend
{
	class D3D12GpuQueue;
	class D3D12Device;
	class SwapChain;
	class CommitedResource;

	class ResourceManager
	{
	public:
						ResourceManager(D3D12Device* device);
		virtual			~ResourceManager();

		void			Update();

		SwapChain*		CreateSwapChain(u32 windowId, HWND windowHandle, const Vec2u& size, const int32_t frameCount);
		SwapChain*		GetSwapChain(u32 windowId) const;
		const std::map<u32, SwapChain*>&
						GetSwapChains() const { return mSwapChains; }

		std::unique_ptr<GI::IGraphicMemoryResource>	CreateResource(const D3D12_RESOURCE_DESC& desc, D3D12_HEAP_TYPE heapType, const char* name, D3D12_RESOURCE_STATES currentState);
		std::unique_ptr<GI::IGraphicMemoryResource>	CreateResource(const GI::MemoryResourceDesc& desc);
		std::unique_ptr<GI::IGraphicMemoryResource>	CreateResource(ID3D12Resource* resource, const char* name, D3D12_RESOURCE_STATES currentState);
		void				ReleaseResource(GI::CommittedResourceId id);
		CommitedResource*	GetResource(GI::CommittedResourceId id) const;

		DescriptorPtr	CreateSrvDescriptor(GI::CommittedResourceId resourceId, const GI::SrvDesc& desc);
		DescriptorPtr	CreateUavDescriptor(GI::CommittedResourceId resourceId, const GI::UavDesc& desc);
		DescriptorPtr	CreateRtvDescriptor(GI::CommittedResourceId resourceId, const GI::RtvDesc& desc);
		DescriptorPtr	CreateDsvDescriptor(GI::CommittedResourceId resourceId, const GI::DsvDesc& desc);
		DescriptorPtr	CreateSampler(const GI::SamplerDesc& desc);

	protected:

		struct ReleaseItem
		{
			GI::CommittedResourceId			mResourceId;
			std::map<D3D12GpuQueue*, u64>	mGpuQueueTimePoints;
		};

		D3D12Device* const			mDevice = nullptr;
		std::vector<ReleaseItem>	mReleaseQueue;

		std::array<D3D12DescriptorAllocator*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mDescAllocator = {};

		std::map<u32, SwapChain*>	mSwapChains;

		IndexAllocator<GI::CommittedResourceId>			mResourceIdAllocator;

		std::map<GI::CommittedResourceId, std::unique_ptr<CommitedResource>, GI::CommittedResourceId::Less> mResourceIdMapping;
	};
}