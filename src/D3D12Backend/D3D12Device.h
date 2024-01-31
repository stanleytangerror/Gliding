#pragma once

#include "D3D12Headers.h"
#include "D3D12GpuQueue.h"
#include "D3D12DescriptorAllocator.h"
#include "D3D12PipelineState.h"
#include "D3D12Shader.h"
#include "D3D12ResourceManager.h"
#include "Common/PresentPort.h"

namespace D3D12Backend
{
	class D3D12PipelineStateLibrary;
	class D3D12ShaderLibrary;
	class D3D12DescriptorAllocator;
	class SwapChain;
	class D3D12GpuQueue;
	class ResourceManager;
	enum D3D12GpuQueueType : u8;

	class GD_D3D12BACKEND_API D3D12Device
	{
	public:
		D3D12Device();

		void	StartFrame();
		void	Present();
		void	Destroy();

		ID3D12Device* GetDevice() const;
		IDXGIFactory4* GetFactory() const;

	public:
		/// <summary>
		/// Post sync process:
		/// 1. release application resources
		/// 2. update resource manager to release d3d12 resources
		/// 3. execute operation
		/// </summary>
		enum PostSyncStage
		{
			PreRelease,
			PostRelease,
			Count
		};
		using PostSyncOperation = std::function<void()>;

		D3D12ShaderLibrary* GetShaderLib() const { return mShaderLib; }
		D3D12PipelineStateLibrary* GetPipelineStateLib() const { return mPipelineStateLib; }
		DescriptorPtr	GetNullSrvUavCbvCpuDesc() const { return mNullSrvCpuDesc; }
		DescriptorPtr	GetNullSamplerCpuDesc() const { return mNullSamplerCpuDesc; }

		D3D12GpuQueue* GetGpuQueue(D3D12GpuQueueType type) const { return mGpuQueues[u64(type)]; }

		ResourceManager* GetResourceManager() const { return mResMgr.get(); }

		void	PushPostSyncOperation(PostSyncStage stage, const PostSyncOperation& operation);

	private:
		IDXGIFactory4* mFactory = nullptr;
		ID3D12Device* mDevice = nullptr;
		
		std::array<D3D12GpuQueue*, u64(D3D12GpuQueueType::Count)>	mGpuQueues;
		std::unique_ptr<ResourceManager>			mResMgr;

		D3D12PipelineStateLibrary* mPipelineStateLib = nullptr;
		D3D12ShaderLibrary* mShaderLib = nullptr;

		DescriptorPtr	mNullSrvCpuDesc;
		DescriptorPtr	mNullSamplerCpuDesc;

		std::array<std::queue<PostSyncOperation>, Count>	mPostSyncQueues;

		static const u32 mSwapChainBufferCount = 3;
	};
}