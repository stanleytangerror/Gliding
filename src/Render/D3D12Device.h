#pragma once

#include "D3D12/D3D12Headers.h"
#include "DescriptorHeap.h"
#include "SuspendedRelease.h"
#include "D3D12DescriptorAllocator.h"
#include <array>

class D3D12Device
{
public:
	void	Initial(HWND windowHandle);
	ID3D12Device* GetDevice() const;

public:
	RuntimeDescriptorHeap* GetRuntimeDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
	D3D12DescriptorAllocator* GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

private:
	ID3D12Device* mDevice = nullptr;
	IDXGISwapChain3* m_swapChain = nullptr;
	ID3D12CommandQueue* m_commandQueue = nullptr;

	using CommandAllocatorPool = SuspendedReleasePool<ID3D12CommandAllocator>;
	CommandAllocatorPool* mCommandAllocatorPool = nullptr;

	std::array<RuntimeDescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mRuntimeDescHeaps = {};
	std::array<D3D12DescriptorAllocator*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mDescAllocator = {};
};