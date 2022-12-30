#pragma once

#include "D3D12Headers.h"

namespace D3D12Backend
{
	class D3D12GpuQueue;
	class D3D12Device;

	class D3D12ResourceManager
	{
	public:
		D3D12ResourceManager(D3D12Device* device);
		virtual ~D3D12ResourceManager();

		void	ReleaseResource(ID3D12Resource* res);
		void	Update();

	protected:

		struct ReleaseItem
		{
			ID3D12Resource* mRes = nullptr;
			std::map<D3D12GpuQueue*, u64> mGpuQueueTimePoints;
		};

		D3D12Device* const			mDevice = nullptr;
		std::vector<ReleaseItem>	mReleaseQueue;
	};
}