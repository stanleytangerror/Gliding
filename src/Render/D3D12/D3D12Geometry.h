#pragma once

#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12Device.h"

class D3D12Geometry
{
public:
	template <typename TVertex>
	static D3D12Geometry* GenerateGeometry(D3D12Device* device, 
		const std::vector<TVertex>& vertices, 
		const std::vector<u16>& indices, 
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& mInputDescs);

	static D3D12Geometry* GenerateQuad(D3D12Device* device);

public:
	ID3D12Resource* mVb = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbv = {};

	ID3D12Resource* mIb = nullptr;
	D3D12_INDEX_BUFFER_VIEW mIbv = {};

	std::vector < D3D12_INPUT_ELEMENT_DESC > mInputDescs;
};

#include "D3D12/D3D12Geometry_inl.h"