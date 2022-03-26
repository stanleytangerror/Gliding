#pragma once

struct GD_WORLD_API TextureRawData
{
	std::string mPath;
	std::vector<b8>	mRawData;
};

struct GD_WORLD_API MatParamMeta
{
	std::string	mName;
	u32			mIndex = 0;
	
};

enum GD_WORLD_API TextureUsage
{
	TextureUsage_Invalid,
	TextureUsage_Diffuse,
	TextureUsage_Specular,
	TextureUsage_Ambient,
	TextureUsage_Emissive,
	TextureUsage_Height,
	TextureUsage_Normal,
	TextureUsage_Shininess,
	TextureUsage_Opacity,
	TextureUsage_Displacement,
	TextureUsage_LightMap,
	TextureUsage_Reflection,
	
	TextureUsage_BaseColor,
	TextureUsage_NormalCamera,
	TextureUsage_EmissiveColor,
	TextureUsage_Metalness,
	TextureUsage_DiffuseRoughness,
	TextureUsage_AmbientOcclusion,
	TextureUsage_Sheen,
	TextureUsage_ClearCoat,
	TextureUsage_Transmission,

	TextureUsage_Custom,
	
	TextureUsage_Count,
};

struct GD_WORLD_API MaterialRawData
{
	std::string mName;
	std::vector<std::string> mTexturePaths;
	std::array<std::vector<std::string>, TextureUsage_Count> mTexturesWithUsage;
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
	enum Semantic : u8 { Position, Normal, Tangent, BiTangent, TexCoord, Color, Semantic_Count };

	Semantic	mType;
	u32			mChannelIndex;
	u32			mStrideInBytes;

	class Less
	{
	public:
		bool operator()(VertexAttriMeta const& o0, VertexAttriMeta const& o1) const
		{
			return 
				o0.mType < o1.mType ? true :
				o0.mType > o1.mType ? false :
				o0.mChannelIndex < o1.mChannelIndex ? true :
				o0.mChannelIndex > o1.mChannelIndex ? false :
				o0.mStrideInBytes < o1.mStrideInBytes;
		}
	};
};

struct GD_WORLD_API MeshRawData
{
	i32	mVertexCount = 0;
	std::map<VertexAttriMeta, std::vector<VertexAttriRawData>, VertexAttriMeta::Less> mVertexData;
	std::vector<Vec3i> mFaces;

	u32	mMaterialIndex = 0;
};

struct GD_WORLD_API SceneRawData
{
	std::vector<MeshRawData*> mMeshes;
	std::map<std::string, TextureRawData*> mTextures;
	std::vector<MaterialRawData*> mMaterials;

	static SceneRawData* LoadScene(const char* path);
};