#include "RenderPch.h"
#include "D3D12/D3D12Geometry.h"

D3D12Geometry::D3D12Geometry(D3D12Device* device)
	: mDevice(device)
{

}

D3D12Geometry::~D3D12Geometry()
{
	mDevice->ReleaseD3D12Resource(mVb);
	mDevice->ReleaseD3D12Resource(mIb);
}

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
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		});
}

D3D12Geometry* D3D12Geometry::GenerateSphere(D3D12Device* device, i32 stacks, i32 slices)
{
	std::vector<GeometryUtils::VertexPosNormUv> vertices;
	std::vector<u16> indices;

	// z-up
	for (int i = 0; i <= stacks; i++)
	{
		f32 v = f32(i) / f32(stacks);
		f32 phi = Math::Pi<f32>() * v;
		for (int j = 0; j < slices; j++)
		{
			f32 u = f32(j) / f32(slices);
			f32 theta = 2.f * Math::Pi<f32>() * u;
			f32 x = std::sin(phi) * std::cos(theta);
			f32 y = std::sin(phi) * std::sin(theta);
			f32 z = std::cos(phi);
			vertices.push_back({ {x, y, z}, {}, {u, v} });
		}
	}

	for (auto& v : vertices)
	{
		v.mNorm = v.mPos.normalized();
	}

	// ccw
	for (int i = 1; i <= stacks; i++)
	{
		for (int j = 0; j < slices; j++)
		{
			auto Idx = [&](i32 stackIdx, i32 sliceIdx) { return stackIdx * slices + sliceIdx; };

			if (i != stacks)
			{
				indices.push_back(Idx(i - 1, j));
				indices.push_back(Idx(i, j));
				indices.push_back(Idx(i, (j + 1) % slices));
			}

			if (i != 1)
			{
				indices.push_back(Idx(i - 1, j));
				indices.push_back(Idx(i, (j + 1) % slices));
				indices.push_back(Idx(i - 1, (j + 1) % slices));
			}
		}
	}

	return D3D12Geometry::GenerateGeometry<GeometryUtils::VertexPosNormUv>(device, vertices, indices, GeometryUtils::VertexPosNormUv::GetInputDesc());
}

//////////////////////////////////////////////////////////////////////////

std::vector<D3D12_INPUT_ELEMENT_DESC> GeometryUtils::VertexPosNormUv::GetInputDesc()
{
	return
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}
