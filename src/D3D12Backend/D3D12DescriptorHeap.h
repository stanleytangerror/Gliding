#pragma once

#include "Common/Pool.h"
#include "D3D12Headers.h"

namespace D3D12Backend
{
	class DescriptorArrayResource
	{
	public:
		DescriptorArrayResource(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const bool deviceVisible, const i32 capacity, const char* name);
		virtual								~DescriptorArrayResource();

		CD3DX12_CPU_DESCRIPTOR_HANDLE		GetCpuBaseWithOffset(i32 index) const;
		CD3DX12_GPU_DESCRIPTOR_HANDLE		GetGpuBaseWithOffset(i32 index) const;
		ID3D12DescriptorHeap*				GetDescriptorHeap() const { return mDescriptorHeap; }
		i32									GetCapacity() const { return mCapacity; }

	protected:
		const D3D12_DESCRIPTOR_HEAP_DESC	mDesc;
		ID3D12DescriptorHeap*				mDescriptorHeap = nullptr; // ownership
		const i32							mCapacity = 0;
		const i32							mStride = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE			mCpuBase = {};
		D3D12_GPU_DESCRIPTOR_HANDLE			mGpuBase = {};
	};

	class RuntimeDescriptorHeap
	{
	public:
		RuntimeDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);
		virtual								~RuntimeDescriptorHeap();

		CD3DX12_GPU_DESCRIPTOR_HANDLE		Push(const i32 handleCount, const D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescHandles);

		ID3D12DescriptorHeap* GetCurrentDescriptorHeap() const { return mCurrentWorkingBlock ? mCurrentWorkingBlock->GetDescriptorHeap() : nullptr; }

		u32									GetDescHandleSize() const;

		void								Reset();

	protected:
		static const i32					msNumDescriptorsPerBlock = 32;
		ID3D12Device* mDevice = nullptr;

		const D3D12_DESCRIPTOR_HEAP_TYPE	mDescriptorType = {};
		Pool<DescriptorArrayResource>			mPool;

		DescriptorArrayResource* mCurrentWorkingBlock = nullptr;
		i32									mCurrentWorkingIndex = 0;
	};
}