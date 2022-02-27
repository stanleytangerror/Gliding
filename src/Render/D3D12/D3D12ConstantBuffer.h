#pragma once

#include "D3D12/D3D12Device.h"

class D3D12Device;

class D3D12ConstantBuffer
{
public:
	D3D12ConstantBuffer(D3D12Device* device);
	virtual							~D3D12ConstantBuffer();

	void							Retire();
	void							Reset();

	void							Submit(const void* data, int size);

	D3D12_GPU_VIRTUAL_ADDRESS		GetWorkGpuVa() const;

private:
	// ring buffer
	int								mWorkingSize = 0;
	int								mTailOffset = 0; // always align 256

	// res
	int								mGpuResourceSize = 0;
	ID3D12Resource* mGpuResource = nullptr; // ownership, release by fence

	// va
	void* mCpuBaseVirtualAddr = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS		mGpuBaseVirtualAddr = {};
};

