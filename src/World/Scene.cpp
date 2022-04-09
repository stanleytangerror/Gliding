#include "WorldPch.h"
#include "Scene.h"

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/importer.hpp>
#include <assimp/postprocess.h>

namespace AssimpLoadUtils
{
	constexpr MaterialParamSemantic FromAssimpTextureUsage(const aiTextureType type)
	{
		switch (type)
		{
		case aiTextureType_NONE				: return TextureUsage_Count;
		case aiTextureType_DIFFUSE 			: return TextureUsage_Diffuse;
		case aiTextureType_SPECULAR			: return TextureUsage_Specular;
		case aiTextureType_AMBIENT 			: return TextureUsage_Ambient;
		case aiTextureType_EMISSIVE			: return TextureUsage_Emissive;
		case aiTextureType_HEIGHT			: return TextureUsage_Height;
		case aiTextureType_NORMALS 			: return TextureUsage_Normal;
		case aiTextureType_SHININESS		: return TextureUsage_Shininess;
		case aiTextureType_OPACITY			: return TextureUsage_Opacity;
		case aiTextureType_DISPLACEMENT		: return TextureUsage_Displacement;
		case aiTextureType_LIGHTMAP			: return TextureUsage_LightMap;
		case aiTextureType_REFLECTION		: return TextureUsage_Reflection;
		case aiTextureType_BASE_COLOR		: return TextureUsage_BaseColor;
		case aiTextureType_NORMAL_CAMERA 	: return TextureUsage_NormalCamera;
		case aiTextureType_EMISSION_COLOR	: return TextureUsage_EmissiveColor;
		case aiTextureType_METALNESS		: return TextureUsage_Metalness;
		case aiTextureType_DIFFUSE_ROUGHNESS: return TextureUsage_Roughness;
		case aiTextureType_AMBIENT_OCCLUSION: return TextureUsage_AmbientOcclusion;
		case aiTextureType_SHEEN			: return TextureUsage_Sheen;
		case aiTextureType_CLEARCOAT		: return TextureUsage_ClearCoat;
		case aiTextureType_TRANSMISSION		: return TextureUsage_Transmission;
		case aiTextureType_UNKNOWN			: return TextureUsage_Custom;
		default								: Assert(false); return TextureUsage_Invalid;
		}
	}

	Vec4f DefaultValueFromSemantic(const MaterialParamSemantic semantic)
	{
		switch (semantic)
		{
		case TextureUsage_Invalid				: return Vec4f::Zero();
		case TextureUsage_Diffuse				: return Vec4f::Ones();
		case TextureUsage_Specular				: return Vec4f::Ones();
		case TextureUsage_Ambient				: return Vec4f::Ones();
		case TextureUsage_Emissive				: return Vec4f::Ones();
		case TextureUsage_Height				: return Vec4f::Ones();
		case TextureUsage_Normal				: return Vec4f(0.5f, 0.5f, 1.f, 0.f);
		case TextureUsage_Shininess				: return Vec4f::Ones();
		case TextureUsage_Opacity				: return Vec4f::Ones();
		case TextureUsage_Displacement			: return Vec4f::Ones();
		case TextureUsage_LightMap				: return Vec4f::Ones();
		case TextureUsage_Reflection			: return Vec4f::Ones();
		case TextureUsage_BaseColor				: return Vec4f::Ones();
		case TextureUsage_NormalCamera			: return Vec4f::Ones();
		case TextureUsage_EmissiveColor			: return Vec4f::Ones();
		case TextureUsage_Metalness				: return Vec4f::Ones();
		case TextureUsage_Roughness				: return Vec4f::Ones();
		case TextureUsage_AmbientOcclusion		: return Vec4f::Ones();
		case TextureUsage_Sheen					: return Vec4f::Ones();
		case TextureUsage_ClearCoat				: return Vec4f::Ones();
		case TextureUsage_Transmission			: return Vec4f::Ones();
		case TextureUsage_Custom				: return Vec4f::Zero();
		default									: Assert(false); return Vec4f::Zero();
		}
	}
	MaterialParamSemantic FromAssimpMatParamKey(const aiString& key)
	{
		const std::string keyStr = key.C_Str();

		if (std::strncmp(keyStr.c_str(), "$clr.", 5) == 0)
		{
			const std::string& colorProp = keyStr.substr(5);

#define CMP_NAME_RET(NAME, ret) if (strncmp(colorProp.c_str(), NAME, strlen(NAME)) == 0) { return ret; }
			CMP_NAME_RET("diffuse"			, TextureUsage_Diffuse			);
			CMP_NAME_RET("specular"			, TextureUsage_Specular			);
			//CMP_NAME_RET("Ambient"			, TextureUsage_Ambient			);
			CMP_NAME_RET("emissive"			, TextureUsage_Emissive			);
			//CMP_NAME_RET("Height"			, TextureUsage_Height			);
			//CMP_NAME_RET("Normal"			, TextureUsage_Normal			);
			CMP_NAME_RET("shininess"		, TextureUsage_Shininess		);
			//CMP_NAME_RET("Opacity"			, TextureUsage_Opacity			);
			//CMP_NAME_RET("Displacement"		, TextureUsage_Displacement		);
			//CMP_NAME_RET("LightMap"			, TextureUsage_LightMap			);
			//CMP_NAME_RET("Reflection"		, TextureUsage_Reflection		);
			CMP_NAME_RET("base"				, TextureUsage_BaseColor		);
			//CMP_NAME_RET("NormalCamera"		, TextureUsage_NormalCamera		);
			//CMP_NAME_RET("EmissiveColor"	, TextureUsage_EmissiveColor	);
			CMP_NAME_RET("metallicFactor"	, TextureUsage_Metalness		);
			CMP_NAME_RET("roughnessFactor"	, TextureUsage_Roughness		);
			//CMP_NAME_RET("AmbientOcclusion"	, TextureUsage_AmbientOcclusion	);
			//CMP_NAME_RET("Sheen"			, TextureUsage_Sheen			);
			//CMP_NAME_RET("ClearCoat"		, TextureUsage_ClearCoat		);
			//CMP_NAME_RET("Transmission"		, TextureUsage_Transmission		);
#undef CMP_NAME_RET
		}

		return TextureUsage_Invalid;
	}

	constexpr TextureSamplerType FromAssimpTextureSamplerType(const std::array<aiTextureMapMode, 2>& type)
	{
		auto mapType = [](aiTextureMapMode type)
		{
			switch (type)
			{
			case aiTextureMapMode_Wrap: return TextureSamplerType_Wrap;
			case aiTextureMapMode_Clamp: return TextureSamplerType_Clamp;
			case aiTextureMapMode_Decal: return TextureSamplerType_Decal;
			case aiTextureMapMode_Mirror: return TextureSamplerType_Mirror;
			default: Assert(false); return TextureSamplerType_Wrap;
			}
		};

		return { mapType(type[0]), mapType(type[1]), TextureSamplerType_Wrap };
	}

	Mat33f GetSceneAlignTransform(
		const Math::Axis3D sourceUpDir,
		const Math::Chirality sourceChirality,
		const Math::Axis3D targetUpDir,
		const Math::Chirality targetChirality)
	{
		if (sourceUpDir == targetUpDir && sourceChirality == targetChirality)
		{
			return Mat33f::Identity();
		}

		const Mat33f rot = Math::GetRotation(sourceUpDir, targetUpDir, sourceChirality);
		if (sourceChirality == targetChirality)
		{
			return rot;
		}

		const Math::Axis3D upAxis = Math::Axis3D((targetUpDir / 2) * 2);
		const Math::Axis3D axisToInvert = Math::Axis3D((upAxis + 2) % 6);
		const Mat33f invertChirality = Scalingf(-1 * axisToInvert);

		return invertChirality * rot;
	}

	MeshRawData* LoadMesh(aiMesh* mesh)
	{
		if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
		{
			//Assert(false);
			return nullptr;
		}

		auto ToVec2f = [](const aiVector3D& v)
		{
			VertexAttriRawData res;
			res.mVec2f.x() = v.x;
			res.mVec2f.y() = v.y;
			return res;
		};

		auto ToVec3f = [](const aiVector3D& v)
		{
			VertexAttriRawData res;
			res.mVec3f.x() = v.x;
			res.mVec3f.y() = v.y;
			res.mVec3f.z() = v.z;
			return res;
		};

		auto ToVec4f = [](const aiColor4D& v)
		{
			VertexAttriRawData res;
			res.mVec4f.x() = v.r;
			res.mVec4f.y() = v.g;
			res.mVec4f.z() = v.b;
			res.mVec4f.w() = v.a;
			return res;
		};


		MeshRawData* result = new MeshRawData;

		result->mVertexCount = mesh->mNumVertices;
		result->mMaterialIndex = mesh->mMaterialIndex;

		if (mesh->HasPositions())
		{
			VertexAttriMeta meta = { VertexAttriMeta::Position, 0, sizeof(Vec3f) };

			std::vector<VertexAttriRawData>& positions = result->mVertexData[meta];
			for (i32 i = 0; i < static_cast<i32>(mesh->mNumVertices); i++)
			{
				positions.push_back(ToVec3f(mesh->mVertices[i]));
			}
		}

		if (mesh->HasNormals())
		{
			VertexAttriMeta meta = { VertexAttriMeta::Normal, 0, sizeof(Vec3f) };

			std::vector<VertexAttriRawData>& normals = result->mVertexData[meta];
			for (i32 i = 0; i < static_cast<i32>(mesh->mNumVertices); i++)
			{
				normals.push_back(ToVec3f(mesh->mNormals[i]));
			}
		}

		if (mesh->HasTangentsAndBitangents())
		{
			VertexAttriMeta metaTan = { VertexAttriMeta::Tangent, 0, sizeof(Vec3f) };
			VertexAttriMeta metaBiTan = { VertexAttriMeta::BiTangent, 0, sizeof(Vec3f) };

			std::vector<VertexAttriRawData>& tangents = result->mVertexData[metaTan];
			std::vector<VertexAttriRawData>& biTangents = result->mVertexData[metaBiTan];

			for (i32 i = 0; i < static_cast<i32>(mesh->mNumVertices); i++)
			{
				tangents.push_back(ToVec3f(mesh->mTangents[i]));
				biTangents.push_back(ToVec3f(mesh->mBitangents[i]));
			}
		}

		for (i32 channel = 0; channel < static_cast<i32>(mesh->GetNumUVChannels()); ++channel)
		{
			if (mesh->HasTextureCoords(channel))
			{
				i32 vecWidth = mesh->mNumUVComponents[channel];
				Assert(vecWidth <= 4);

				VertexAttriMeta meta = { VertexAttriMeta::TexCoord, channel, static_cast<i32>(sizeof(f32) * vecWidth) };

				std::vector<VertexAttriRawData>& data = result->mVertexData[meta];
				for (i32 i = 0; i < static_cast<i32>(mesh->mNumVertices); i++)
				{
					switch (vecWidth)
					{
					case 2:
						data.push_back(ToVec2f(mesh->mTextureCoords[channel][i]));
						break;
					case 3:
						data.push_back(ToVec3f(mesh->mTextureCoords[channel][i]));
						break;
					default:
						Assert(false);
						break;
					}

				}
			}
		}

		for (i32 channel = 0; channel < static_cast<i32>(mesh->GetNumColorChannels()); ++channel)
		{
			if (mesh->HasVertexColors(channel))
			{
				VertexAttriMeta meta = { VertexAttriMeta::Color, channel, sizeof(Vec4f) };

				std::vector<VertexAttriRawData>& data = result->mVertexData[meta];
				for (i32 i = 0; i < static_cast<i32>(mesh->mNumVertices); i++)
				{
					data.push_back(ToVec4f(mesh->mColors[channel][i]));
				}
			}
		}

		result->mFaces.resize(mesh->mNumFaces);
		for (i32 i = 0; i < static_cast<i32>(mesh->mNumFaces); i++)
		{
			aiFace face = mesh->mFaces[i];

			for (i32 j = 0; j < static_cast<i32>(face.mNumIndices); j++)
			{
				result->mFaces[i][j] = face.mIndices[j];
			}
		}

		return result;
	}

	TextureRawData* LoadTexture(const std::string& texPath)
	{
		return new TextureRawData
		{
			texPath,
			Utils::LoadFileContent(texPath.c_str()),
		};
	}

	MaterialRawData* LoadMaterial(const std::filesystem::path& folder, aiMaterial* material)
	{
		MaterialRawData* result = new MaterialRawData;

		result->mName = material->GetName().C_Str();
		for (i32 t = 0; t < TextureUsage_Count; ++t)
		{
			result->mParamSemanticSlots[t].mConstantValue = DefaultValueFromSemantic(MaterialParamSemantic(t));
		}

		for (i32 t = 0; t < aiTextureType_UNKNOWN; ++t)
		{
			const aiTextureType texType = aiTextureType(t);
			for (i32 idx = 0; idx < material->GetTextureCount(texType); ++idx)
			{
				aiString relPath;
				std::array<aiTextureMapMode, 2> mapmode = {};
				if (AI_SUCCESS == material->GetTexture(texType, idx, &relPath, nullptr, nullptr, nullptr, nullptr, mapmode.data()))
				{
					MaterialRawData::ParamBasicInfo& info = result->mParamSemanticSlots[FromAssimpTextureUsage(texType)];
					info.mTexturePath = Utils::ToString(folder / relPath.C_Str());
					info.mSamplerType = FromAssimpTextureSamplerType(mapmode);
				}
			}
		}

		for (i32 i = 0; i < material->mNumProperties; ++i)
		{
			aiMaterialProperty* prop = material->mProperties[i];

			if (prop->mSemantic != aiTextureType_NONE) { continue; }

			MaterialParamSemantic sem = FromAssimpMatParamKey(prop->mKey);
			if (sem == TextureUsage_Invalid) { continue; }

			const char* propName = prop->mKey.C_Str();
			MaterialRawData::ParamBasicInfo& info = result->mParamSemanticSlots[sem];

			switch (prop->mType)
			{
			case aiPTI_Float:
			case aiPTI_Double:
			{
				aiColor4D c4; aiColor3D c3; ai_real f;
				if (AI_SUCCESS == material->Get(propName, prop->mSemantic, prop->mIndex, c4))
				{
					info.mConstantValue = Vec4f(c4.r, c4.g, c4.b, c4.a);
				}
				else if (AI_SUCCESS == material->Get(propName, prop->mSemantic, prop->mIndex, c3))
				{
					info.mConstantValue = Vec4f(c3.r, c3.g, c3.b, 1.f);
				}
				else if (AI_SUCCESS == material->Get(propName, prop->mSemantic, prop->mIndex, f))
				{
					info.mConstantValue = Vec4f::Ones() * f32(f);
				}
				break;
			}
			case aiPTI_String:
			{
				aiString str;
				if (AI_SUCCESS == material->Get(propName, prop->mSemantic, prop->mIndex, str))
				{
					result->mStringParams[propName] = str.C_Str();
				}
				break;
			}
			default:
				break;
			}
		}

		return result;
	}

	Mat44f FromAssimpMatrix(const aiMatrix4x4& aiMat44)
	{
		// in memory, aiMatrix4x4 is rol major, Mat44 is col major
		aiMatrix4x4 m = aiMat44;
		m.Transpose();

		const Mat44f result = *(Mat44f*)&(m);
		return result;
	}

	void CalcMeshTransforms(aiNode* node, const Mat44f& parentTrans, std::map<u32, Mat44f>& transforms)
	{
		if (!node) { return; }

		const Mat44f& absTrans = parentTrans * FromAssimpMatrix(node->mTransformation);

		for (i32 i = 0; i < node->mNumMeshes; ++i)
		{
			u32 meshIdx = node->mMeshes[i];
			Assert(transforms.find(meshIdx) == transforms.end());
			transforms[meshIdx] = absTrans;
		}

		for (i32 i = 0; i < node->mNumChildren; ++i)
		{
			CalcMeshTransforms(node->mChildren[i], absTrans, transforms);
		}
	}
}

SceneRawData* SceneRawData::LoadScene(const char* path, 
	const Math::Axis3D sourceUpDir,
	const Math::Chirality sourceChirality,
	const Math::Axis3D targetUpDir,
	const Math::Chirality targetChirality)
{
	SceneRawData* scene = new SceneRawData;

	Assimp::Importer importer;

	//const aiScene* pScene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Quality);
	const aiScene* pScene = importer.ReadFile(path, 
		aiProcess_CalcTangentSpace |
		//aiProcess_GenSmoothNormals |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ImproveCacheLocality |
		aiProcess_LimitBoneWeights |
		aiProcess_RemoveRedundantMaterials |
		//aiProcess_SplitLargeMeshes |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_SortByPType |
		aiProcess_FindDegenerates |
		aiProcess_FindInvalidData
		);
	if (!pScene) { return nullptr; }

	const std::filesystem::path& fileFolder = std::filesystem::path(path).parent_path();

	std::map<u32, Mat44f> transforms;
	Mat44f alignTransform = Mat44f::Identity();
	alignTransform.block(0, 0, 3, 3) = AssimpLoadUtils::GetSceneAlignTransform(sourceUpDir, sourceChirality, targetUpDir, targetChirality);
	AssimpLoadUtils::CalcMeshTransforms(pScene->mRootNode, alignTransform, transforms);

	for (u32 meshIdx = 0; meshIdx < pScene->mNumMeshes; ++meshIdx)
	{
		if (MeshRawData* mesh = AssimpLoadUtils::LoadMesh(pScene->mMeshes[meshIdx]))
		{
			Assert(transforms.find(meshIdx) != transforms.end());
			mesh->mTransform = transforms[meshIdx];
			scene->mMeshes.emplace_back(mesh);
		}
	}

	std::set<std::string> texturePaths;
	for (int i = 0; i < static_cast<i32>(pScene->mNumMaterials); ++i)
	{
		if (MaterialRawData* mat = AssimpLoadUtils::LoadMaterial(fileFolder, pScene->mMaterials[i]))
		{
			scene->mMaterials.emplace_back(mat);

			for (const auto& slot : mat->mParamSemanticSlots)
			{
				if (!slot.mTexturePath.empty())
				{
					texturePaths.insert(slot.mTexturePath);
					scene->mSamplers.insert(slot.mSamplerType);
				}
			}
		}
	}

	for (const auto& path : texturePaths)
	{
		if (TextureRawData* textureRawData = AssimpLoadUtils::LoadTexture(path))
		{
			scene->mTextures[path] = textureRawData;
		}
	}

	return scene;
}
