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

		std::unique_ptr<GI::IGraphicMemoryResource>	CreateResource(const GI::MemoryResourceDesc& desc);
		std::unique_ptr<GI::IGraphicMemoryResource>	PossessResourceWithOwnership(ID3D12Resource* resource, const char* name, D3D12_RESOURCE_STATES currentState);
		void				ReleaseResource(GI::CommittedResourceId id);
		CommitedResource*	GetResource(GI::CommittedResourceId id) const;
		CommitedResource*	GetResource(const GI::IGraphicMemoryResource* resource) const;

		DescriptorPtr	CreateSrvDescriptor(GI::CommittedResourceId resourceId, const GI::SrvDesc& desc);
		DescriptorPtr	CreateUavDescriptor(GI::CommittedResourceId resourceId, const GI::UavDesc& desc);
		DescriptorPtr	CreateRtvDescriptor(GI::CommittedResourceId resourceId, const GI::RtvDesc& desc);
		DescriptorPtr	CreateDsvDescriptor(GI::CommittedResourceId resourceId, const GI::DsvDesc& desc);
		DescriptorPtr	CreateSampler(const GI::SamplerDesc& desc);

		void			ReleaseAllSamplers();

	protected:

		struct ReleaseItem
		{
			GI::CommittedResourceId			mResourceId;
			std::map<D3D12GpuQueue*, u64>	mGpuQueueTimePoints;
		};

		D3D12Device* const			mDevice = nullptr;
		std::vector<ReleaseItem>	mReleaseQueue;

		std::array<std::unique_ptr<D3D12DescriptorAllocator>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mDescAllocator;

		IndexAllocator<GI::CommittedResourceId>			mResourceIdAllocator;

		using HashValue = u32;
		std::map<GI::CommittedResourceId, std::unique_ptr<CommitedResource>, GI::CommittedResourceId::Less> mResourceIdMapping;
		std::map<GI::CommittedResourceId, std::map<HashValue, std::pair<D3D12DescriptorAllocator*, DescriptorPtr>>, GI::CommittedResourceId::Less> mResourceViewMapping;
		std::map<HashValue, std::pair<D3D12DescriptorAllocator*, DescriptorPtr>> mSamplerMapping;

		struct ResourceMonitor
		{
		public:
			void OnCreateResource(ID3D12Resource* resource, const D3D12_RESOURCE_DESC& desc, const char* name);
			void OnPossessResourceWithOwnership(ID3D12Resource* resource, const D3D12_RESOURCE_DESC& desc, const char* name);
			void OnReleaseResource(ID3D12Resource* resource);

			void PrintResourceStatistics();

		private:
			static u32	CalcMemorySize(const D3D12_RESOURCE_DESC& desc);

			struct DeviceResourceStatus
			{
				std::string mName;
				D3D12_RESOURCE_DESC mDesc;
				u32	mMemorySize = 0;
			};

			std::map<ID3D12Resource*, DeviceResourceStatus>	mResources;
		};

		ResourceMonitor			mMonitor;
	};
}