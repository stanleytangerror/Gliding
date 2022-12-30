#pragma once

#include "D3D12Headers.h"
#include "D3D12Device.h"

struct MeshRawData;

class GD_D3D12BACKEND_API D3D12Geometry
{
protected:
	D3D12Geometry(D3D12Backend::D3D12Device* device);

public:
	D3D12Backend::D3D12Device* const		mDevice = nullptr;

	std::unique_ptr< D3D12Backend::CommitedResource> mVb = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbv = {};

	std::unique_ptr< D3D12Backend::CommitedResource> mIb = nullptr;
	D3D12_INDEX_BUFFER_VIEW mIbv = {};

	std::vector < D3D12_INPUT_ELEMENT_DESC > mInputDescs;

public:
	template <typename TVertex>
	static D3D12Geometry* GenerateGeometry(D3D12Backend::D3D12Device* device,
		const std::vector<TVertex>& vertices,
		const std::vector<u16>& indices,
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs);

	static D3D12Geometry* GenerateQuad(D3D12Backend::D3D12Device* device);
	static D3D12Geometry* GenerateSphere(D3D12Backend::D3D12Device* device, i32 subDev);

	static D3D12Geometry* GenerateGeometry(D3D12Backend::D3D12Device* device,
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

	struct VertexPosNormTanUv
	{
		Vec3f mPos;
		Vec3f mNorm;
		Vec3f mTangent;
		Vec3f mBiTangent;
		Vec2f mUv;

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputDesc();
	};
}

#include "D3D12Geometry_inl.h"