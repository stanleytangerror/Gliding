#pragma once

struct GD_WORLD_API TextureRawData
{

};

union GD_WORLD_API VertexAttriRawData
{
	VertexAttriRawData() { memset(this, 0, sizeof(*this)); }
	VertexAttriRawData(const VertexAttriRawData& other) { memcpy(this, &other, sizeof(*this)); }
	VertexAttriRawData(VertexAttriRawData&& other) { memcpy(this, &other, sizeof(*this)); }
	VertexAttriRawData& operator=(const VertexAttriRawData& other) { memcpy(this, &other, sizeof(*this)); return *this; }
	VertexAttriRawData& operator=(VertexAttriRawData&& other) { memcpy(this, &other, sizeof(*this)); return *this; }

	Vec2i mVec2i;
	Vec2f mVec2f;
	Vec3f mVec3f;
	Vec3i mVec3i;
	Vec4f mVec4f;
};

struct GD_WORLD_API VertexAttriMeta
{
	enum Semantic : u8 { Position, Normal, Tangent, BiTangent, TexCoord };

	Semantic	mType;
	i32			mChannelIndex;
	i32			mStrideInBytes;

	class Hash
	{
	public:
		std::size_t operator()(VertexAttriMeta const& o) const
		{
			std::size_t h1 = std::hash<Semantic>()(o.mType);
			std::size_t h2 = std::hash<i32>()(o.mChannelIndex);
			std::size_t h3 = std::hash<i32>()(o.mStrideInBytes);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	class Equal
	{
	public:
		bool operator()(VertexAttriMeta const& o0, VertexAttriMeta const& o1) const
		{
			return memcmp(&o0, &o1, sizeof(o0));
		}
	};
};

struct GD_WORLD_API MeshRawData
{
	std::unordered_map<VertexAttriMeta, std::vector<VertexAttriRawData>, VertexAttriMeta::Hash, VertexAttriMeta::Equal> mVertexData;
	std::vector<Vec3i> mFaces;
};

struct GD_WORLD_API SceneRawData
{
	std::vector<MeshRawData*> mModels;
	std::vector<TextureRawData*> mTextures;

	static SceneRawData* LoadScene(const char* path);
};