#include "RenderPch.h"
#include "Geometry.h"
#include "D3D12Backend/D3D12Resource.h"
#include "Common/GraphicsInfrastructure.h"

void Geometry::CreateAndInitialResource(GI::IGraphicsInfra* infra)
{
	mVb = infra->CreateMemoryResource(
		GI::MemoryResourceDesc()
		.SetAlignment(0)
		.SetDimension(GI::ResourceDimension::BUFFER)
		.SetWidth(mVertices.size())
		.SetHeight(1)
		.SetDepthOrArraySize(1)
		.SetMipLevels(1)
		.SetFormat(GI::Format::FORMAT_UNKNOWN)
		.SetLayout(GI::TextureLayout::LAYOUT_ROW_MAJOR)
		.SetFlags(GI::ResourceFlags::NONE)
		.SetInitState(GI::ResourceState::STATE_GENERIC_READ)
		.SetHeapType(GI::HeapType::UPLOAD));

	infra->CopyToUploadMemoryResource(mVb.get(), mVertices);

	mIb = infra->CreateMemoryResource(
		GI::MemoryResourceDesc()
		.SetAlignment(0)
		.SetDimension(GI::ResourceDimension::BUFFER)
		.SetWidth(mIndices.size() * sizeof(u16))
		.SetHeight(1)
		.SetDepthOrArraySize(1)
		.SetMipLevels(1)
		.SetFormat(GI::Format::FORMAT_UNKNOWN)
		.SetLayout(GI::TextureLayout::LAYOUT_ROW_MAJOR)
		.SetFlags(GI::ResourceFlags::NONE)
		.SetInitState(GI::ResourceState::STATE_GENERIC_READ)
		.SetHeapType(GI::HeapType::UPLOAD));

	std::vector<b8> buf(mIndices.size() * sizeof(u16));
	std::memcpy(buf.data(), mIndices.data(), buf.size());
	infra->CopyToUploadMemoryResource(mIb.get(), buf);
}

Geometry* Geometry::GenerateQuad()
{
	return Geometry::GenerateGeometry<Vec2f>(
		{
			Vec2f{ 1.f, -1.f },
			Vec2f{ 1.f, 1.f },
			Vec2f{ -1.f, 1.f },
			Vec2f{ -1.f, -1.f } },
		{ 0, 1, 2, 0, 2, 3 },
		{
			GI::InputElementDesc()
				.SetSemanticName("POSITION")
				.SetSemanticIndex(0)
				.SetFormat(GI::Format::FORMAT_R32G32_FLOAT)
				.SetInputSlot(0)
				.SetAlignedByteOffset(0)
				.SetInputSlotClass(GI::InputClassification::PER_VERTEX_DATA)
				.SetInstanceDataStepRate(0)
		});
}

Geometry* Geometry::GenerateSphere(i32 subDev)
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

	return Geometry::GenerateGeometry<GeometryUtils::VertexPosNormTanUv>(device, vertices, indices, GeometryUtils::VertexPosNormTanUv::GetInputDesc());
}


Geometry* Geometry::GenerateGeometry(const std::vector<b8>& vertices, i32 vertexStride, const std::vector<u16>& indices, const std::vector<GI::InputElementDesc>& inputDescs)
{
	Geometry* result = new Geometry;

	result->mVertices = vertices;
	result->mVertexStride = vertexStride;
	result->mIndices = indices;
	result->mVertexElementDescs = inputDescs;

	return result;
}

//////////////////////////////////////////////////////////////////////////

std::vector<GI::InputElementDesc> GeometryUtils::VertexPosNormUv::GetInputDesc()
{
	return
	{
		GI::InputElementDesc()
				.SetSemanticName("POSITION")
				.SetFormat(GI::Format::FORMAT_R32G32B32_FLOAT)
				.SetAlignedByteOffset(0),
		GI::InputElementDesc()
				.SetSemanticName("NORMAL")
				.SetFormat(GI::Format::FORMAT_R32G32B32_FLOAT)
				.SetAlignedByteOffset(12),
		GI::InputElementDesc()
				.SetSemanticName("TEXCOORD")
				.SetFormat(GI::Format::FORMAT_R32G32_FLOAT)
				.SetAlignedByteOffset(24)
	};
}

std::vector<GI::InputElementDesc> GeometryUtils::VertexPosNormTanUv::GetInputDesc()
{
	return
	{
		GI::InputElementDesc()
				.SetSemanticName("POSITION")
				.SetFormat(GI::Format::FORMAT_R32G32B32_FLOAT)
				.SetAlignedByteOffset(0),
		GI::InputElementDesc()
				.SetSemanticName("NORMAL")
				.SetFormat(GI::Format::FORMAT_R32G32B32_FLOAT)
				.SetAlignedByteOffset(12),
		GI::InputElementDesc()
				.SetSemanticName("TANGENT")
				.SetFormat(GI::Format::FORMAT_R32G32B32_FLOAT)
				.SetAlignedByteOffset(24),
		GI::InputElementDesc()
				.SetSemanticName("BINORMAL")
				.SetFormat(GI::Format::FORMAT_R32G32B32_FLOAT)
				.SetAlignedByteOffset(36),
		GI::InputElementDesc()
				.SetSemanticName("TEXCOORD")
				.SetFormat(GI::Format::FORMAT_R32G32_FLOAT)
				.SetAlignedByteOffset(48)
	};
}
