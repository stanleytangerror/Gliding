#include "RenderPch.h"
#include "D3D12/D3D12Geometry.h"

D3D12Geometry* D3D12Geometry::GenerateQuad(D3D12Device* device)
{
	return D3D12Geometry::GenerateGeometry<Vec2f>(device,
		{
			Vec2f{ 1.f, -1.f },
			Vec2f{ 1.f, 1.f },
			Vec2f{ -1.f, 1.f },
			Vec2f{ -1.f, -1.f } },
		{ 0, 1, 2, 0, 2, 3 },
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		});
}
