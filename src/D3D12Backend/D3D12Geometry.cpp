#include "D3D12BackendPch.h"
#include "D3D12Geometry.h"

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

D3D12Geometry* D3D12Geometry::GenerateSphere(D3D12Device* device, i32 subDev)
{
	std::vector<GeometryUtils::VertexPosNormTanUv> vertices;
	std::vector<u16> indices;

	const i32 stacks = subDev;
	const i32 slices = 2 * subDev;

	// z-up
	for (int i = 0; i <= stacks; i++)
	{
		f32 v = f32(i) / f32(stacks);
		f32 theta = Math::Pi<f32>() * v;
		for (int j = 0; j < slices; j++)
		{
			f32 u = f32(j) / f32(slices);
			f32 phi = 2.f * Math::Pi<f32>() * u;

			const Vec3f pos = {
				std::sin(theta) * std::cos(phi),
				std::sin(theta) * std::sin(phi),
				std::cos(theta) };
			
			const Vec3f norm = pos.normalized();
			const Vec3f tang = Vec3f{ -std::sin(phi), std::cos(phi), 0.f }.normalized();
			const Vec3f biTang = norm.cross(tang);

			vertices.push_back({ pos, norm, tang, biTang, {u, v} });
		}
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

	return D3D12Geometry::GenerateGeometry<GeometryUtils::VertexPosNormTanUv>(device, vertices, indices, GeometryUtils::VertexPosNormTanUv::GetInputDesc());
}


D3D12Geometry* D3D12Geometry::GenerateGeometry(D3D12Device* device, const std::vector<b8>& vertices, i32 vertexStride, const std::vector<u16>& indices, const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs)
{
	D3D12Geometry* result = new D3D12Geometry(device);

	result->mVb = D3D12Utils::CreateUploadBuffer(device->GetDevice(), vertices.size());
	result->mIb = D3D12Utils::CreateUploadBuffer(device->GetDevice(), indices.size() * sizeof(u16));

	{
		u8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		AssertHResultOk(result->mVb->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, vertices.data(), vertices.size());
	}

	{
		u8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		AssertHResultOk(result->mIb->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices.data(), indices.size() * sizeof(u16));
	}

	result->mInputDescs = inputDescs;

	result->mVbv.BufferLocation = result->mVb->GetGPUVirtualAddress();
	result->mVbv.StrideInBytes = vertexStride;
	result->mVbv.SizeInBytes = u32(vertices.size());

	result->mIbv.BufferLocation = result->mIb->GetGPUVirtualAddress();
	result->mIbv.SizeInBytes = u32(indices.size() * sizeof(u16));
	result->mIbv.Format = DXGI_FORMAT_R16_UINT;

	return result;
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

std::vector<D3D12_INPUT_ELEMENT_DESC> GeometryUtils::VertexPosNormTanUv::GetInputDesc()
{
	return
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}
