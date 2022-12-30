#pragma once

#include "D3D12Geometry.h"

template <typename TVertex>
D3D12Geometry* D3D12Geometry::GenerateGeometry(D3D12Backend::D3D12Device* device,
	const std::vector<TVertex>& vertices,
	const std::vector<u16>& indices,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs)
{
	std::vector<b8> rawVertices(vertices.size() * sizeof(TVertex));
	memcpy(rawVertices.data(), vertices.data(), vertices.size() * sizeof(TVertex));
	return GenerateGeometry(device, rawVertices, sizeof(TVertex), indices, inputDescs);
}
