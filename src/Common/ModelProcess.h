#pragma once

#include "CommonTypes.h"
#include "Serialization.h"
#include "Math.h"

#pragma pack(push, 1) // all the content should be pack 1 to avoid memory alignment issue

namespace ModelProcess
{
	struct GD_COMMON_API Guid
	{
		i32 a;
		i16 b;
		i16 c;
		std::array<b8, 8> d;
	};

	static_assert(std::is_trivially_copyable_v<Guid>, "Guid should be trivially copyable");

	CLASS_SERIALIZE_BYTES(Guid, a, b, c, d);

	enum GD_COMMON_API VertexSemantic : u16
	{ 
		Position, Normal, Tangent, BiTangent, TexCoord, Color, Semantic_Count 
	};

	enum GD_COMMON_API ScalarType : u16
	{
		Float, Double, Int32, UInt32, Int16, UInt16
	};

	struct GD_COMMON_API VertexAttributeMeta
	{
		VertexSemantic mSemantic;
		u16 mSemanticIndex;
		ScalarType mScalarType;
		u16 mSizeInBytes;
		u16 mOffsetInBytes;
	};

	CLASS_SERIALIZE_BYTES(VertexAttributeMeta, mSemantic, mSemanticIndex, mScalarType, mSizeInBytes, mOffsetInBytes);

	struct GD_COMMON_API Channel
	{
		std::string Name;
		std::string TexturePath;
		i32 TexCoord;
		std::vector<std::pair<std::string, float>> ScalarParams;
		std::vector<std::pair<std::string, Vec2f>> Vector2Params;
		std::vector<std::pair<std::string, Vec3f>> Vector3Params;
		std::vector<std::pair<std::string, Vec4f>> Vector4Params;
	};

	CLASS_SERIALIZE_BYTES(Channel, Name, TexturePath, TexCoord, ScalarParams, Vector2Params, Vector3Params, Vector4Params);

	struct GD_COMMON_API Material
	{
		Guid Id;
		std::string Name;
		std::vector<Channel> Channels;
	};

	CLASS_SERIALIZE_BYTES(Material, Id, Name, Channels);

	struct GD_COMMON_API Mesh
	{
		std::string Name;
		Guid MaterialId;

		std::vector<VertexAttributeMeta> VertexAttributeMetas;
		std::vector<b8> Vertices;
		std::vector<u16> Indices;
	};

	CLASS_SERIALIZE_BYTES(Mesh, Name, MaterialId, VertexAttributeMetas, Vertices, Indices);

	struct GD_COMMON_API Model
	{
		std::string Name;
		std::vector<Material> Materials;
		std::vector<Mesh> Meshes;
	};

	CLASS_SERIALIZE_BYTES(Model, Name, Materials, Meshes);
}

#pragma pack(pop)
