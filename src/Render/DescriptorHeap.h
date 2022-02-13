#pragma once

#include "SuspendedRelease.h"
#include "Common/CommonTypes.h"
#include "D3D12/D3D12Headers.h"

class DescriptorHeapBlock
{
public:
	DescriptorHeapBlock(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const bool shaderVisible, const i32 numDescriptors);
	virtual ~DescriptorHeapBlock();

	CD3DX12_CPU_DESCRIPTOR_HANDLE			GetCpuBaseWithOffset(i32 offset) const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE			GetGpuBaseWithOffset(i32 offset) const;
	ID3D12DescriptorHeap*				GetDescriptorHeap() const { return m_DescriptorHeap; }
	i32									GetNumDescriptos() const { return mDescriptorNum; }

protected:
	const D3D12_DESCRIPTOR_HEAP_DESC	mDesc;
	ID3D12DescriptorHeap*				m_DescriptorHeap = nullptr; // ownership
	const i32							m_DescriptorSize = 0;
	const i32							mDescriptorNum = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE			m_CpuBase = {};
	D3D12_GPU_DESCRIPTOR_HANDLE			m_GpuBase = {};
};

class RuntimeDescriptorHeap
{
public:
	RuntimeDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);
	virtual								~RuntimeDescriptorHeap();

	void								Push(const int32_t handleCount, const D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescHandles, const u64 fenceValue);
	void								Retire(const int32_t handleCount);

	ID3D12DescriptorHeap*				GetCurrentDescriptorHeap() const { return mCurrentWorkingBlock ? mCurrentWorkingBlock->GetDescriptorHeap() : nullptr; }
	CD3DX12_GPU_DESCRIPTOR_HANDLE		GetGpuHandle(const int32_t offset) const;

	void								UpdateCompletedFenceValue(u64 val);

protected:
	static const i32					msNumDescriptorsPerBlock = 32;
	ID3D12Device*						mDevice = nullptr;

	const D3D12_DESCRIPTOR_HEAP_TYPE	mDescriptorType = {};
	SuspendedReleasePool<DescriptorHeapBlock> mPool;

	DescriptorHeapBlock* mCurrentWorkingBlock = nullptr;
	i32					mCurrentWorkingIndex = 0;
};