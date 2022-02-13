#pragma once

#include "D3D12/D3D12Headers.h"
#include "DescriptorHeap.h"
#include "SuspendedRelease.h"
#include <array>

class D3D12Device
{
public:
	void	Initial(HWND windowHandle);

public:

private:
	ID3D12Device* m_device = nullptr;
	IDXGISwapChain3* m_swapChain = nullptr;
	ID3D12CommandQueue* m_commandQueue = nullptr;

	using CommandAllocatorPool = SuspendedReleasePool<ID3D12CommandAllocator>;
	CommandAllocatorPool* mCommandAllocatorPool = nullptr;

	std::array<RuntimeDescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mRuntimeDescHeaps = {};
};