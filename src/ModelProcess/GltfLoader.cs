using System.Numerics;
using GLTF = SharpGLTF.Schema2;

// using SharpGLTF: https://github.com/vpenades/SharpGLTF/tree/master
// code sample: https://github.com/vpenades/MonoScene/blob/db9f693a4f29a16bf9e13939797a0d37f60c55e2/src/MonoScene.Pipeline.GLTF/Factories/GLTFMaterialsFactory.cs

namespace ModelProcess
{
    public class GltfLoader
    {
        static TResult[]? GetVertices<TResult>(GLTF.MeshPrimitive mesh, VertexSemantic semantic, int number = 0)
        {
            var key = semantic switch
            {
                VertexSemantic.Position => "POSITION",
                VertexSemantic.Normal => "NORMAL",
                VertexSemantic.Tangent => "TANGENT",
                VertexSemantic.BiTangent => "BITANGENT",
                VertexSemantic.TexCoord => $"TEXCOORD_{number}",
                VertexSemantic.Color => "COLOR",
                _ => throw new NotImplementedException()
            };

            if (mesh.VertexAccessors.TryGetValue(key, out var accessor))
            {
                if (accessor.Format.Encoding != GLTF.EncodingType.FLOAT)
                {
                    throw new NotImplementedException();
                }

                switch (accessor.Format.Dimensions)
                {
                    case GLTF.DimensionType.VEC4:
                        return accessor!.AsVector4Array().Select(v => v.Truncate<TResult>()).ToArray();
                    case GLTF.DimensionType.VEC3:
                        return accessor!.AsVector3Array().Select(v => v.Truncate<TResult>()).ToArray();
                    case GLTF.DimensionType.VEC2:
                        return accessor!.AsVector2Array().Select(v => v.Truncate<TResult>()).ToArray();
                    default:
                        throw new NotImplementedException();
                }
            }

            return null;
        }

        public static Model Load(string path)
        {
            var srcModel = GLTF.ModelRoot.Load(path);

            var srcTexToDstTex = srcModel.LogicalTextures.ToDictionary(tex => tex, LoadTexture);
            var srcMatToDstMat = srcModel.LogicalMaterials.ToDictionary(mat => mat, mat => LoadMaterial(mat, a => srcTexToDstTex[a]));

            Model dstModel = new()
            {
                Name = path,
                Textures = srcTexToDstTex
                    .Select(p => p.Value)
                    .ToArray(),
                Materials = srcMatToDstMat
                    .Select(p => p.Value)
                    .ToArray(),
                Meshes = srcModel.LogicalMeshes
                    .SelectMany(m => m.Primitives)
                    .Select(p => LoadMeshPrimitive(p, a => srcMatToDstMat[a]))
                    .ToArray()
            };

            return dstModel;
        }

        protected static TextureSamplerData LoadSampler(GLTF.TextureSampler srcSampler)
        {
            return new TextureSamplerData
            {
                MinFilter = srcSampler.MinFilter switch
                {
                    GLTF.TextureMipMapFilter.LINEAR => TextureInterpolationFilter.Linear,
                    GLTF.TextureMipMapFilter.NEAREST => TextureInterpolationFilter.Point,
                    GLTF.TextureMipMapFilter.LINEAR_MIPMAP_NEAREST => TextureInterpolationFilter.Linear,
                    GLTF.TextureMipMapFilter.LINEAR_MIPMAP_LINEAR => TextureInterpolationFilter.Linear,
                    GLTF.TextureMipMapFilter.NEAREST_MIPMAP_NEAREST => TextureInterpolationFilter.Point,
                    GLTF.TextureMipMapFilter.NEAREST_MIPMAP_LINEAR => TextureInterpolationFilter.Point,
                    _ => throw new NotImplementedException(),
                },
                MagFilter = srcSampler.MagFilter switch
                {
                    GLTF.TextureInterpolationFilter.LINEAR => TextureInterpolationFilter.Linear,
                    GLTF.TextureInterpolationFilter.NEAREST => TextureInterpolationFilter.Point,
                    _ => throw new NotImplementedException(),
                },
                MipMapFilter = srcSampler.MinFilter switch
                {
                    GLTF.TextureMipMapFilter.LINEAR_MIPMAP_NEAREST => TextureInterpolationFilter.Point,
                    GLTF.TextureMipMapFilter.LINEAR_MIPMAP_LINEAR => TextureInterpolationFilter.Linear,
                    GLTF.TextureMipMapFilter.NEAREST_MIPMAP_NEAREST => TextureInterpolationFilter.Point,
                    GLTF.TextureMipMapFilter.NEAREST_MIPMAP_LINEAR => TextureInterpolationFilter.Linear,
                    _ => throw new NotImplementedException(),
                },
                AddressMode = [
                    GetAddressMode(srcSampler.WrapS),
                    GetAddressMode(srcSampler.WrapT),
                    TextureAddressMode.Wrap
                ]
            };

            static TextureAddressMode GetAddressMode(GLTF.TextureWrapMode wrapMode)
            {
                return wrapMode switch
                {
                    GLTF.TextureWrapMode.REPEAT => TextureAddressMode.Wrap,
                    GLTF.TextureWrapMode.CLAMP_TO_EDGE => TextureAddressMode.Clamp,
                    GLTF.TextureWrapMode.MIRRORED_REPEAT => TextureAddressMode.Mirror,
                    _ => throw new NotImplementedException()
                };
            }
        }

        protected static Texture LoadTexture(GLTF.Texture srcTexture)
        {
            return new Texture
            {
                Path = srcTexture.PrimaryImage?.Content.SourcePath,
                Sampler = LoadSampler(srcTexture.Sampler),
            };
        }

        protected static Material LoadMaterial(GLTF.Material srcMaterial, Func<GLTF.Texture, Texture> getDstTexture)
        {
            var dstMaterial = new Material
            {
                Name = srcMaterial.Name,
                Channels = srcMaterial.Channels.Select(srcChannel =>
                {
                    Material.Channel dstChannel = new();
                    dstChannel.Name = srcChannel.Key;
                    dstChannel.Texture = srcChannel.Texture is null ? null : getDstTexture(srcChannel.Texture);
                    dstChannel.TexCoord = srcChannel.TextureCoordinate;
                    dstChannel.ScalarParams = srcChannel.Parameters
                        .Where(p => p.Value is float)
                        .ToDictionary(p => p.Name, p => (float) p.Value);
                    dstChannel.Vector2Params = srcChannel.Parameters
                        .Where(p => p.Value is Vector2)
                        .ToDictionary(p => p.Name, p => (Vector2)p.Value);
                    dstChannel.Vector3Params = srcChannel.Parameters
                        .Where(p => p.Value is Vector3)
                        .ToDictionary(p => p.Name, p => (Vector3)p.Value);
                    dstChannel.Vector4Params = srcChannel.Parameters
                        .Where(p => p.Value is Vector4)
                        .ToDictionary(p => p.Name, p => (Vector4)p.Value);

                    return dstChannel;
                }).ToArray()
            };

            return dstMaterial;
        }

        protected static Mesh LoadMeshPrimitive(GLTF.MeshPrimitive srcMesh, Func<GLTF.Material, Material> getDstMaterial)
        {
            if (srcMesh.DrawPrimitiveType != GLTF.PrimitiveType.TRIANGLES)
            {
                throw new Exception($"Invalid primitive type {srcMesh.DrawPrimitiveType}");
            }

            Mesh dstMesh = new()
            {
                Name = $"{srcMesh.LogicalParent.Name}_Primitive",
                Material = getDstMaterial(srcMesh.Material),
                Indices = [.. srcMesh.GetIndices()],
                Positions = GetVertices<Vector3>(srcMesh, VertexSemantic.Position) ?? [],
                Normals = GetVertices<Vector3>(srcMesh, VertexSemantic.Normal) ?? [],
                Tangents = GetVertices<Vector3>(srcMesh, VertexSemantic.Tangent) ?? [],
                BiTangents = GetVertices<Vector3>(srcMesh, VertexSemantic.BiTangent) ?? [],
                TexCoords = Enumerable.Range(0, 4)
                    .Select(i => GetVertices<Vector2>(srcMesh, VertexSemantic.TexCoord, number: i) ?? [])
                    .Where(arr => arr is not null)
                    .ToList(),
                Colors = GetVertices<Vector4>(srcMesh, VertexSemantic.Color) ?? [],
            };

            return dstMesh;
        }
    }
}
