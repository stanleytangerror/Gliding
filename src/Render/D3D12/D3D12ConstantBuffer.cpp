#include "RenderPch.h"
#include "D3D12ConstantBuffer.h"

namespace
{
	template <typename T>
	constexpr T GpuVaAlign(T Val)
	{
		return Math::Align(Val, 256);
	}
}

D3D12ConstantBuffer::D3D12ConstantBuffer(D3D12Device* device)
	: mDevice(device)
{
	const int ConstBuffSize = 32 * 1024;

	mGpuResourceSize = GpuVaAlign(ConstBuffSize);

	CD3DX12_HEAP_PROPERTIES heapUpload(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC uploadBuffer = CD3DX12_RESOURCE_DESC::Buffer(mGpuResourceSize);
	device->GetDevice()->CreateCommittedResource(
		&heapUpload,
		D3D12_HEAP_FLAG_NONE,
		&uploadBuffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mGpuResource));

	mGpuBaseVirtualAddr = mGpuResource->GetGPUVirtualAddress();
	assert(mGpuBaseVirtualAddr == GpuVaAlign(mGpuBaseVirtualAddr));

	mGpuResource->Map(0, nullptr, &mCpuBaseVirtualAddr);
}

D3D12ConstantBuffer::~D3D12ConstantBuffer()
{
	mDevice->ReleaseD3D12Resource(mGpuResource);
}

void D3D12ConstantBuffer::Submit(const void* data, int size)
{
	if (0 == size)
	{
		return;
	}

	const int alignedSize = GpuVaAlign(size);
	if (mTailOffset + alignedSize >= mGpuResourceSize)
	{
		Reset();
		return;
	}

	mWorkingSize = alignedSize;

	void* dstPtr = (char*)mCpuBaseVirtualAddr + mTailOffset;
	memcpy(dstPtr, data, size);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBuffer::GetWorkGpuVa() const
{
	return mGpuBaseVirtualAddr + mTailOffset;
}

void D3D12ConstantBuffer::Retire()
{
	if (mWorkingSize == 0)
	{
		return;
	}

	int headOffset = mTailOffset + mWorkingSize;
	if (headOffset >= mGpuResourceSize)
	{
		headOffset = 0;
	}
	mTailOffset = GpuVaAlign(headOffset);

	mWorkingSize = 0;
}

void D3D12ConstantBuffer::Reset()
{
	mWorkingSize = 0;
	mTailOffset = 0;
}