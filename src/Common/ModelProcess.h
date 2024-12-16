#pragma once

#include "CommonTypes.h"
#include "Serialization.h"
#include "Math.h"
#include "GraphicsInfrastructure.h"

#pragma pack(push, 1) // all the content should be pack 1 to avoid memory alignment issue

struct GD_COMMON_API Guid
{
	u32 a;
	u16 b;
	u16 c;
	u64 d;

	static Guid Default()
	{
		return {};
	}

	bool IsValid() const
	{
		return a == 0 && b == 0 && c == 0 && d == 0;
	}
};
static_assert(std::is_trivially_copyable_v<Guid>, "Guid should be trivially copyable");

CLASS_SERIALIZE_BYTES(Guid, a, b, c, d);

// Specialize std::hash for Guid
namespace std
{
	template <>
	struct hash<Guid>
	{
		std::size_t operator()(const Guid& guid) const
		{
			std::size_t h1 = std::hash<decltype(Guid::a)>{}(guid.a);
			std::size_t h2 = std::hash<decltype(Guid::b)>{}(guid.b);
			std::size_t h3 = std::hash<decltype(Guid::c)>{}(guid.c);
			std::size_t h4 = std::hash<decltype(Guid::d)>{}(guid.d);
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
		}
	};

	inline bool operator<(const Guid& lhs, const Guid& rhs)
	{
		return std::tie(lhs.a, lhs.b, lhs.c, lhs.d) < std::tie(rhs.a, rhs.b, rhs.c, rhs.d);
	}

	inline bool operator==(const Guid& lhs, const Guid& rhs)
	{
		return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c && lhs.d == rhs.d;
	}
}

namespace ModelProcess
{
	enum GD_COMMON_API VertexSemantic : u16
	{ 
		Position, Normal, Tangent, BiTangent, TexCoord, Color, Semantic_Count 
	};

	enum GD_COMMON_API ScalarType : u16
	{
		Float, Double, Int32, UInt32, Int16, UInt16
	};

	enum GD_COMMON_API TextureInterpolationFilter : u16
	{
		Point, Linear
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

	struct GD_COMMON_API Sampler
	{
		TextureInterpolationFilter mMinFilter;
		TextureInterpolationFilter mMagFilter;
		TextureInterpolationFilter mMipMapFilter;
		std::array<GI::TextureAddressMode::Enum, 3> mAddressMode;
	};

	CLASS_SERIALIZE_BYTES(Sampler, mMinFilter, mMagFilter, mMipMapFilter, mAddressMode);

	struct GD_COMMON_API Texture
	{
		Guid mId;
		std::string mPath;
		Sampler mSampler;
	};

	CLASS_SERIALIZE_BYTES(Texture, mId, mPath, mSampler);

	struct GD_COMMON_API Channel
	{
		std::string Name;
		Guid TextureId;
		i32 TexCoord;
		std::vector<std::pair<std::string, float>> ScalarParams;
		std::vector<std::pair<std::string, Vec2f>> Vector2Params;
		std::vector<std::pair<std::string, Vec3f>> Vector3Params;
		std::vector<std::pair<std::string, Vec4f>> Vector4Params;
	};

	CLASS_SERIALIZE_BYTES(Channel, Name, TextureId, TexCoord, ScalarParams, Vector2Params, Vector3Params, Vector4Params);

	struct GD_COMMON_API Material
	{
		Guid Id;
		std::string Name;
		std::vector<Channel> Channels;
	};

	CLASS_SERIALIZE_BYTES(Material, Id, Name, Channels);

	struct GD_COMMON_API Mesh
	{
		Guid Id;
		std::string Name;
		Guid MaterialId;

		std::vector<VertexAttributeMeta> VertexAttributeMetas;
		std::vector<b8> Vertices;
		std::vector<u16> Indices;
	};

	CLASS_SERIALIZE_BYTES(Mesh, Id, Name, MaterialId, VertexAttributeMetas, Vertices, Indices);

	struct GD_COMMON_API MeshInstance
	{
		Guid MeshId;
		Mat44f LocalTransform;
	};

	CLASS_SERIALIZE_BYTES(MeshInstance, MeshId, LocalTransform);

	struct GD_COMMON_API Model
	{
		std::string Name;
		std::vector<Texture> Textures;
		std::vector<Material> Materials;
		std::vector<Mesh> Meshes;
		std::vector<MeshInstance> MeshInstances;
	};

	CLASS_SERIALIZE_BYTES(Model, Name, Textures, Materials, Meshes, MeshInstances);

	GD_COMMON_API float GetScalarParam(const Channel& channel, const char* name, float defaultValue = 0.0f);
	GD_COMMON_API Vec2f GetVec2fParam(const Channel& channel, const char* name, const Vec2f& defaultValue = Vec2f::Zero());
	GD_COMMON_API Vec3f GetVec3fParam(const Channel& channel, const char* name, const Vec3f& defaultValue = Vec3f::Zero());
	GD_COMMON_API Vec4f GetVec4fParam(const Channel& channel, const char* name, const Vec4f& defaultValue = Vec4f::Zero());
}

#pragma pack(pop)
