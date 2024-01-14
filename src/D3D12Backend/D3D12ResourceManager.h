#pragma once

#include "D3D12Headers.h"
#include "D3D12Backend/D3D12DescriptorAllocator.h"

namespace D3D12Backend
{
	class D3D12GpuQueue;
	class D3D12Device;

	class ResourceManager
	{
	public:
		using CreateResrouce = std::function<ID3D12Resource* (ID3D12Device*)>;

	public:
						ResourceManager(D3D12Device* device);
		virtual			~ResourceManager();

		ID3D12Resource* CreateResource(const CreateResrouce& builder);
		void			ReleaseResource(ID3D12Resource* res);
		void			Update();

		DescriptorPtr	CreateSrvDescriptor(ID3D12Resource* res, const GI::SrvDesc& desc);
		DescriptorPtr	CreateUavDescriptor(ID3D12Resource* res, const GI::UavDesc& desc);
		DescriptorPtr	CreateRtvDescriptor(ID3D12Resource* res, const GI::RtvDesc& desc);
		DescriptorPtr	CreateDsvDescriptor(ID3D12Resource* res, const GI::DsvDesc& desc);
		DescriptorPtr	CreateSampler(const GI::SamplerDesc& desc);

	protected:

		struct ReleaseItem
		{
			ID3D12Resource* mRes = nullptr;
			std::map<D3D12GpuQueue*, u64> mGpuQueueTimePoints;
		};

		D3D12Device* const			mDevice = nullptr;
		std::vector<ReleaseItem>	mReleaseQueue;

		std::array<D3D12DescriptorAllocator*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mDescAllocator = {};
	};
}