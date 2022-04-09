#include "RenderPch.h"
#include "D3D12ConstantBuffer.h"

ConstBufferBlock::ConstBufferBlock(D3D12Device* device, i32 size)
	: mDevice(device)
	, mSize(Math::Align(size, msGpuAddrAlignment))
{
	CD3DX12_HEAP_PROPERTIES heapUpload(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC uploadBuffer = CD3DX12_RESOURCE_DESC::Buffer(mSize);
	device->GetDevice()->CreateCommittedResource(
		&heapUpload,
		D3D12_HEAP_FLAG_NONE,
		&uploadBuffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mGpuResource));

	mGpuBaseVirtualAddr = mGpuResource->GetGPUVirtualAddress();
	Assert(mGpuBaseVirtualAddr == Math::Align(mGpuBaseVirtualAddr, msGpuAddrAlignment));

	void* mapCpuAddr = nullptr;
	mGpuResource->Map(0, nullptr, &mapCpuAddr);
	mCpuBaseVirtualAddr = reinterpret_cast<byte*>(mapCpuAddr);
}

ConstBufferBlock::~ConstBufferBlock()
{
	mDevice->ReleaseD3D12Resource(mGpuResource);
}

D3D12_GPU_VIRTUAL_ADDRESS ConstBufferBlock::Push(const void* data, i32 size)
{
	// Alloc at lease 1, if alloc 0 then nothing will be alloced. 
	// If mWorkingSize == mSize, then return end() which is out of bounc
	const i32 sizeOnGpu = Math::Align(std::max<i32>(1, size), msGpuAddrAlignment);
	if (mWorkingSize + sizeOnGpu <= mSize)
	{
		memcpy(mCpuBaseVirtualAddr + mWorkingSize, data, size);
		D3D12_GPU_VIRTUAL_ADDRESS result = mGpuBaseVirtualAddr + mWorkingSize;
		mWorkingSize += sizeOnGpu;
		return result;
	}
	else
	{
		return D3D12_GPU_VIRTUAL_ADDRESS(0);
	}
}

void ConstBufferBlock::Reset()
{
	mWorkingSize = 0;
}

//////////////////////////////////////////////////////////////////////////

D3D12ConstantBuffer::D3D12ConstantBuffer(D3D12Device* device)
	: mDevice(device)
	, mPool(
		[device]() { return new ConstBufferBlock(device, msBlockSize); },
		[](ConstBufferBlock* t) { t->Reset(); },
		[](ConstBufferBlock* t) { Utils::SafeDelete(t); }
	)
{

}

D3D12ConstantBuffer::~D3D12ConstantBuffer()
{
	Reset();
}

void D3D12ConstantBuffer::Reset()
{
	mPool.ReleaseAllActiveItems();
	mWorkingBlock = nullptr;
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBuffer::Push(const void* data, i32 size)
{
	Assert(size <= msBlockSize);

	if (mWorkingBlock)
	{
		D3D12_GPU_VIRTUAL_ADDRESS result = mWorkingBlock->Push(data, size);
		if (result != D3D12_GPU_VIRTUAL_ADDRESS(0))
		{
			return result;
		}
	}

	mWorkingBlock = mPool.AllocItem();
	D3D12_GPU_VIRTUAL_ADDRESS result = mWorkingBlock->Push(data, size);
	Assert(result != D3D12_GPU_VIRTUAL_ADDRESS(0));
	return result;
}
