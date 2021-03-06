#pragma once

#include "D3D12Headers.h"
#include "D3D12Device.h"

struct MeshRawData;

class GD_D3D12BACKEND_API D3D12Geometry
{
public:
	D3D12Geometry(D3D12Device* device);
	virtual ~D3D12Geometry();

public:
	D3D12Device* const		mDevice = nullptr;

	ID3D12Resource* mVb = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbv = {};

	ID3D12Resource* mIb = nullptr;
	D3D12_INDEX_BUFFER_VIEW mIbv = {};

	std::vector < D3D12_INPUT_ELEMENT_DESC > mInputDescs;

public:
	template <typename TVertex>
	static D3D12Geometry* GenerateGeometry(D3D12Device* device,
		const std::vector<TVertex>& vertices,
		const std::vector<u16>& indices,
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs);

	static D3D12Geometry* GenerateQuad(D3D12Device* device);
	static D3D12Geometry* GenerateSphere(D3D12Device* device, i32 stacks, i32 slices);

	static D3D12Geometry* GenerateGeometry(D3D12Device* device,
		const std::vector<b8>& vertices, i32 vertexStride,
		const std::vector<u16>& indices,
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs);
};

namespace GeometryUtils
{
	struct VertexPosNormUv
	{
		Vec3f mPos;
		Vec3f mNorm;
		Vec2f mUv;

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputDesc();
	};
}

#include "D3D12Geometry_inl.h"