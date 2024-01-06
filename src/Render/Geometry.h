#pragma once

#include "Common/GraphicsInfrastructure.h"

struct MeshRawData;

class GD_RENDER_API Geometry
{
public:
	void CreateAndInitialResource(GI::IGraphicsInfra* infra);

	bool IsGraphicsResourceReady() const { return !mVb || !mIb; };

	GI::VbvDesc	GetVbvDesc() const;
	GI::IbvDesc	GetIbvDesc() const;

public:
	std::vector<b8>		mVertices;
	i32					mVertexStride = 0;
	std::vector<u16>	mIndices;
	std::vector < GI::InputElementDesc > mVertexElementDescs;

	std::unique_ptr<GI::IGraphicMemoryResource> mVb = nullptr;
	std::unique_ptr<GI::IGraphicMemoryResource> mIb = nullptr;

public:
	template <typename TVertex>
	static Geometry* GenerateGeometry(
		const std::vector<TVertex>& vertices,
		const std::vector<u16>& indices,
		const std::vector<GI::InputElementDesc>& inputDescs);

	static Geometry* GenerateQuad();
	static Geometry* GenerateSphere(i32 subDev);

	static Geometry* GenerateGeometry(
		const std::vector<b8>& vertices, i32 vertexStride,
		const std::vector<u16>& indices,
		const std::vector<GI::InputElementDesc>& inputDescs);
};

namespace GeometryUtils
{
	struct VertexPosNormUv
	{
		Vec3f mPos;
		Vec3f mNorm;
		Vec2f mUv;

		static std::vector<GI::InputElementDesc> GetInputDesc();
	};

	struct VertexPosNormTanUv
	{
		Vec3f mPos;
		Vec3f mNorm;
		Vec3f mTangent;
		Vec3f mBiTangent;
		Vec2f mUv;

		static std::vector<GI::InputElementDesc> GetInputDesc();
	};
}

#include "Geometry_inl.h"