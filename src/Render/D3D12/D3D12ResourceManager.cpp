#include "RenderPch.h"
#include "D3D12ResourceManager.h"
#include "D3D12/D3D12Device.h"

D3D12ResourceManager::D3D12ResourceManager(D3D12Device* device)
	: mDevice(device)
{

}

D3D12ResourceManager::~D3D12ResourceManager()
{
	Assert(mReleaseQueue.empty());
}

void D3D12ResourceManager::ReleaseResource(ID3D12Resource* res)
{
	ReleaseItem item;
	item.mRes = res;

	for (i32 t = 0; t < Count; ++t)
	{
		D3D12GpuQueue* q = mDevice->GetGpuQueue(D3D12GpuQueueType(t));
		item.mGpuQueueTimePoints[q] = q->GetGpuPlannedValue();
	};

	mReleaseQueue.push_back(item);
}

void D3D12ResourceManager::Update()
{
	for (auto it = mReleaseQueue.begin(); it != mReleaseQueue.end();)
	{
		const ReleaseItem& item = *it;
		
		if (std::all_of(item.mGpuQueueTimePoints.begin(), item.mGpuQueueTimePoints.end(), 
			[](const auto& p) { return p.first->IsGpuValueFinished(p.second); }))
		{
			item.mRes->Release();
			it = mReleaseQueue.erase(it);
		}
		else
		{
			++it;
		}
	}
}
