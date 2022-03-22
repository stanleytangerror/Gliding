#include "WorldPch.h"
#include "Scene.h"

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/importer.hpp>
#include <assimp/postprocess.h>

namespace AssimpLoadUtils
{
	MeshRawData* LoadMesh(aiMesh* mesh)
	{
		Assert(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

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

		MeshRawData* result = new MeshRawData;

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

	TextureRawData* LoadTexture(aiTexture* texture)
	{
		return nullptr;
	}
}

SceneRawData* SceneRawData::LoadScene(const char* path)
{
	SceneRawData* scene = new SceneRawData;

	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(path,
		aiProcess_Triangulate |
		aiProcess_ValidateDataStructure |
		aiProcess_CalcTangentSpace /*|
		aiProcess_ConvertToLeftHanded*/);

	if (!pScene) { return nullptr; }

	for (int i = 0; i < static_cast<i32>(pScene->mNumMeshes); ++i)
	{
		aiMesh* mesh = pScene->mMeshes[i];
		scene->mModels.emplace_back(AssimpLoadUtils::LoadMesh(mesh));
	}

	for (int i = 0; i < static_cast<i32>(pScene->mNumTextures); ++i)
	{
		aiTexture* texture = pScene->mTextures[i];
		scene->mTextures.emplace_back(AssimpLoadUtils::LoadTexture(texture));
	}

	return scene;
}
