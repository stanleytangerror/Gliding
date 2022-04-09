#include "WorldPch.h"
#include "Scene.h"

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/importer.hpp>
#include <assimp/postprocess.h>

namespace AssimpLoadUtils
{
	constexpr TextureUsage FromAssimpTextureUsage(const aiTextureType type)
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

	TextureRawData* LoadTexture(const MaterialRawData::TextureBasicInfo& texInfo)
	{
		return new TextureRawData
		{
			texInfo.mTexturePath,
			Utils::LoadFileContent(texInfo.mTexturePath.c_str()),
			texInfo.mSamplerType
		};
	}

	MaterialRawData* LoadMaterial(const std::filesystem::path& folder, aiMaterial* material, std::vector<MaterialRawData::TextureBasicInfo>& textureInfos)
	{
		MaterialRawData* result = new MaterialRawData;

		result->mName = material->GetName().C_Str();

		for (i32 t = 0; t < aiTextureType_UNKNOWN; ++t)
		{
			const aiTextureType texType = aiTextureType(t);
			for (i32 idx = 0; idx < material->GetTextureCount(texType); ++idx)
			{
				aiString relPath;
				std::array<aiTextureMapMode, 2> mapmode = {};
				if (AI_SUCCESS == material->GetTexture(texType, idx, &relPath, nullptr, nullptr, nullptr, nullptr, mapmode.data()))
				{
					const auto& texturePath = Utils::ToString(folder / relPath.C_Str());
					
					const MaterialRawData::TextureBasicInfo& texInfo = { texturePath, FromAssimpTextureSamplerType(mapmode) };
					textureInfos.push_back(texInfo);
					result->mTexturePaths.push_back(texInfo);
					result->mTexturesWithUsage[FromAssimpTextureUsage(texType)].push_back(texInfo);
				}
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

	std::vector<MaterialRawData::TextureBasicInfo> textureInfos;
	for (int i = 0; i < static_cast<i32>(pScene->mNumMaterials); ++i)
	{
		if (MaterialRawData* mat = AssimpLoadUtils::LoadMaterial(fileFolder, pScene->mMaterials[i], textureInfos))
		{
			scene->mMaterials.emplace_back(mat);
		}
	}

	for (const MaterialRawData::TextureBasicInfo& texInfo : textureInfos)
	{
		if (TextureRawData* textureRawData = AssimpLoadUtils::LoadTexture(texInfo))
		{
			scene->mTextures[texInfo.mTexturePath] = textureRawData;
		}
	}

	return scene;
}
