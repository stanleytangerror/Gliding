#pragma once

#include "D3D12Device.h"
#include "Common/Pool.h"

class D3D12Device;

class ConstBufferBlock
{
public:
	ConstBufferBlock(D3D12Device* device, i32 size);
	virtual ~ConstBufferBlock();

	D3D12_GPU_VIRTUAL_ADDRESS		Push(const void* data, i32 size);
	void							Reset();

private:
	static const u64				msGpuAddrAlignment = 256;
	D3D12Device* const				mDevice = nullptr;
	i32	const						mSize = 0;
	
	ID3D12Resource*					mGpuResource = nullptr; // ownership, release by fence

	// va
	byte*							mCpuBaseVirtualAddr = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS		mGpuBaseVirtualAddr = {};

	i32								mWorkingSize = 0;
};

class D3D12ConstantBuffer
{
public:
	D3D12ConstantBuffer(D3D12Device* device);
	virtual							~D3D12ConstantBuffer();

	void							Reset();
	D3D12_GPU_VIRTUAL_ADDRESS		Push(const void* data, i32 size);

private:
	static const i32				msBlockSize = 2048;
	D3D12Device* const				mDevice = nullptr;

	Pool<ConstBufferBlock>			mPool;
	ConstBufferBlock*				mWorkingBlock = nullptr;
};

