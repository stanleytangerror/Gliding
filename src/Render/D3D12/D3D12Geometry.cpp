#include "RenderPch.h"
#include "D3D12/D3D12Geometry.h"
#include "World/Scene.h"

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

D3D12Geometry* D3D12Geometry::GenerateGeometryFromMeshRawData(D3D12Device* device, const MeshRawData* meshRawData)
{
	const i32 vertexCount = meshRawData->mVertexCount;
	const auto& vertexData = meshRawData->mVertexData;
	const u32 vertexTotalStride = std::accumulate(vertexData.begin(), vertexData.end(), 0, [](u32 a, const auto& p) { return a + p.first.mStrideInBytes; });

	std::vector<byte> vertices(vertexTotalStride * vertexCount, 0);
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputDesc;

	static const char* names[] =
	{
		"POSITION",
		"NORMAL",
		"TANGENT",
		"BINORMAL",
		"TEXCOORD",
		"COLOR"
	};

	static const DXGI_FORMAT formats[] =
	{
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
	};

	u32 vertexStride = 0;
	for (const auto& p : vertexData)
	{
		const VertexAttriMeta& meta = p.first;
		const std::vector<VertexAttriRawData>& attrData = p.second;

		inputDesc.push_back(D3D12_INPUT_ELEMENT_DESC{
			names[meta.mType],
			meta.mChannelIndex,
			formats[meta.mStrideInBytes / sizeof(f32)],
			meta.mChannelIndex,
			vertexStride,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 });

		byte* target = vertices.data() + vertexStride;
		for (i32 i = 0; i < vertexCount; i += 1, target += vertexTotalStride)
		{
			memcpy(target, &attrData[i], meta.mStrideInBytes);
		}

		vertexStride += meta.mStrideInBytes;
	}

	std::vector<u16> indices;
	indices.reserve(meshRawData->mFaces.size() * 3);
	for (const auto& f : meshRawData->mFaces)
	{
		indices.push_back(f[0]);
		indices.push_back(f[1]);
		indices.push_back(f[2]);
	}

	return GenerateGeometry(device, vertices, vertexTotalStride, indices, inputDesc);
}

D3D12Geometry* D3D12Geometry::GenerateGeometry(D3D12Device* device, const std::vector<byte>& vertices, i32 vertexStride, const std::vector<u16>& indices, const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs)
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
