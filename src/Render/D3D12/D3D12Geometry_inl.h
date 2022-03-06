#pragma once

#include "D3D12/D3D12Geometry.h"

template <typename TVertex>
D3D12Geometry* D3D12Geometry::GenerateGeometry(D3D12Device* device,
	const std::vector<TVertex>& vertices,
	const std::vector<u16>& indices,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs)
{
	D3D12Geometry* result = new D3D12Geometry;

	result->mVb = D3D12Utils::CreateUploadBuffer(device->GetDevice(), vertices.size() * sizeof(TVertex));
	result->mIb = D3D12Utils::CreateUploadBuffer(device->GetDevice(), indices.size() * sizeof(u16));

	{
		u8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		AssertHResultOk(result->mVb->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, vertices.data(), vertices.size() * sizeof(TVertex));
	}

	{
		u8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		AssertHResultOk(result->mIb->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices.data(), indices.size() * sizeof(u16));
	}

	result->mInputDescs = inputDescs;

	result->mVbv.BufferLocation = result->mVb->GetGPUVirtualAddress();
	result->mVbv.StrideInBytes = sizeof(TVertex);
	result->mVbv.SizeInBytes = u32(vertices.size() * sizeof(TVertex));
	
	result->mIbv.BufferLocation = result->mIb->GetGPUVirtualAddress();
	result->mIbv.SizeInBytes = u32(indices.size() * sizeof(u16));
	result->mIbv.Format = DXGI_FORMAT_R16_UINT;

	return result;
}
