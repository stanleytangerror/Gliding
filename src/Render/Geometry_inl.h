#pragma once

#include "Geometry.h"

template <typename TVertex>
Geometry* Geometry::GenerateGeometry(D3D12Backend::D3D12Device* device,
	const std::vector<TVertex>& vertices,
	const std::vector<u16>& indices,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs)
{
	std::vector<b8> rawVertices(vertices.size() * sizeof(TVertex));
	memcpy(rawVertices.data(), vertices.data(), vertices.size() * sizeof(TVertex));
	return GenerateGeometry(device, rawVertices, sizeof(TVertex), indices, inputDescs);
}

template <typename TVertex>
inline GeometryData* GeometryData::GenerateGeometryData(
	const std::vector<TVertex>& vertices,
	const std::vector<u16>& indices,
	const std::vector<RHI::InputElementDesc>& inputDescs)
{
	std::vector<b8> vertexData(vertices.size() * sizeof(TVertex));
	std::copy(reinterpret_cast<const b8*>(vertices.data()), reinterpret_cast<const b8*>(vertices.data() + vertices.size()), vertexData.data());
	return new GeometryData{ vertexData, indices, inputDescs, u32(vertices.size()), sizeof(TVertex) };
}