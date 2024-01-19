#pragma once

#include "Geometry.h"

template <typename TVertex>
Geometry* Geometry::GenerateGeometry(
	const std::vector<TVertex>& vertices,
	const std::vector<u16>& indices,
	const std::vector<GI::InputElementDesc>& inputDescs)
{
	std::vector<b8> rawVertices(vertices.size() * sizeof(TVertex));
	memcpy(rawVertices.data(), vertices.data(), vertices.size() * sizeof(TVertex));
	return GenerateGeometry(rawVertices, sizeof(TVertex), indices, inputDescs);
}
