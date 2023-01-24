#pragma once

#include "D3D12Backend/D3D12Headers.h"
#include "D3D12Backend/D3D12Device.h"
#include "RenderInterface/RenderTypes.h"

struct MeshRawData;

class GD_RENDER_API Geometry
{
protected:
	Geometry(D3D12Backend::D3D12Device* device);

public:
	D3D12Backend::D3D12Device* const		mDevice = nullptr;

	std::unique_ptr< D3D12Backend::CommitedResource> mVb = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbv = {};

	std::unique_ptr< D3D12Backend::CommitedResource> mIb = nullptr;
	D3D12_INDEX_BUFFER_VIEW mIbv = {};

	std::vector < D3D12_INPUT_ELEMENT_DESC > mInputDescs;

public:
	template <typename TVertex>
	static Geometry* GenerateGeometry(D3D12Backend::D3D12Device* device,
		const std::vector<TVertex>& vertices,
		const std::vector<u16>& indices,
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs);

	static Geometry* GenerateQuad(D3D12Backend::D3D12Device* device);
	static Geometry* GenerateSphere(D3D12Backend::D3D12Device* device, i32 subDev);

	static Geometry* GenerateGeometry(D3D12Backend::D3D12Device* device,
		const std::vector<b8>& vertices, i32 vertexStride,
		const std::vector<u16>& indices,
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputDescs);
};

class GD_RENDER_API GeometryData
{
public:
	std::vector<b8> mVertexData;
	std::vector<u16> mIndexData;
	std::vector<RHI::InputElementDesc> mInputDescs;

public:
	template <typename TVertex>
	static GeometryData* GenerateGeometryData(
		const std::vector<TVertex>& vertices,
		const std::vector<u16>& indices,
		const std::vector<RHI::InputElementDesc>& inputDescs);

	static GeometryData* GenerateQuad();

	static GeometryData* GenerateSphere(i32 subDev);
};

namespace GeometryUtils
{
	struct VertexPosNormUv
	{
		Vec3f mPos;
		Vec3f mNorm;
		Vec2f mUv;

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputDesc();
		static std::vector<RHI::InputElementDesc> GetInputElementsDesc();
	};

	struct VertexPosNormTanUv
	{
		Vec3f mPos;
		Vec3f mNorm;
		Vec3f mTangent;
		Vec3f mBiTangent;
		Vec2f mUv;

		static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputDesc();
		static std::vector<RHI::InputElementDesc> GetInputElementsDesc();
	}; 
}

#include "Geometry_inl.h"