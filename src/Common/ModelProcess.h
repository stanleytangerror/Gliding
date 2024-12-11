#pragma once

#include "CommonTypes.h"
#include "Serialization.h"
#include "Math.h"

namespace ModelProcess
{
	struct GD_COMMON_API Guid
	{
		i32 a;
		i16 b;
		i16 c;
		std::array<b8, 8> d;
	};

	CLASS_SERIALIZE_BYTES(Guid, a, b, c, d);

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
		std::vector<Vec3f> Positions;
		std::vector<Vec3f> Normals;
		std::vector<Vec3f> Tangents;
		std::vector<Vec3f> BiTangents;
		std::vector<std::vector<Vec2f>> TexCoords;
		std::vector<u32> Indices;
	};

	CLASS_SERIALIZE_BYTES(Mesh, Name, MaterialId, Positions, Normals, Tangents, BiTangents, TexCoords, Indices);

	struct GD_COMMON_API Model
	{
		std::string Name;
		std::vector<Material> Materials;
		std::vector<Mesh> Meshes;
	};

	CLASS_SERIALIZE_BYTES(Model, Name, Materials, Meshes);
}