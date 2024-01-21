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
	class Possessor;

	struct CommittedResourceId
	{
		const u64 mId;

		static CommittedResourceId FromInt(u64 v) { return { v }; }

		struct Less
		{
			constexpr bool operator() (const CommittedResourceId& left, const CommittedResourceId& right) const { return left.mId < right.mId; }
		};
	};

	class ResourceManager
	{
	public:
		using CreateResrouce = std::function<ID3D12Resource* (ID3D12Device*)>;

	public:
						ResourceManager(D3D12Device* device);
		virtual			~ResourceManager();

		void			Update();

		SwapChain*		CreateSwapChain(u32 windowId, HWND windowHandle, const Vec2i& size, const int32_t frameCount);
		SwapChain*		GetSwapChain(u32 windowId) const;
		const std::map<u32, SwapChain*>&
						GetSwapChains() const { return mSwapChains; }

		ID3D12Resource* CreateResource(const CreateResrouce& builder);
		std::unique_ptr<CommitedResource>	CreateResource(const GI::MemoryResourceDesc& desc);
		CommitedResource*					CreateResource(const Possessor& possessor);
		void			ReleaseResource(ID3D12Resource* res);

		DescriptorPtr	CreateSrvDescriptor(const GI::SrvDesc& desc);
		DescriptorPtr	CreateUavDescriptor(const GI::UavDesc& desc);
		DescriptorPtr	CreateRtvDescriptor(const GI::RtvDesc& desc);
		DescriptorPtr	CreateDsvDescriptor(const GI::DsvDesc& desc);
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

		std::map<u32, SwapChain*>	mSwapChains;

		IndexAllocator<CommittedResourceId>			mResourceIdAllocator;

		std::map<CommittedResourceId, ID3D12Resource*, CommittedResourceId::Less> mResourceIdMapping;
	};
}