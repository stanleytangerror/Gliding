using System.Collections.Generic;
using System.Linq;
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

            var srcMatToDstMat = srcModel.LogicalMaterials.ToDictionary(material => material, material => LoadMaterial(material));

            Model dstModel = new()
            {
                Name = path,
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

        protected static Material LoadMaterial(GLTF.Material srcMaterial)
        {
            var dstMaterial = new Material
            {
                Name = srcMaterial.Name,
                Channels = srcMaterial.Channels.Select(srcChannel =>
                {
                    Material.Channel dstChannel = new();
                    dstChannel.Name = srcChannel.Key;
                    dstChannel.TexturePath = srcChannel.Texture?.PrimaryImage?.Content.SourcePath ?? string.Empty;
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
                Material = getDstMaterial(srcMesh.Material),
                Positions = GetVertices<Vector3>(srcMesh, VertexSemantic.Position),
                Normals = GetVertices<Vector3>(srcMesh, VertexSemantic.Normal),
                Tangents = GetVertices<Vector3>(srcMesh, VertexSemantic.Tangent),
                BiTangents = GetVertices<Vector3>(srcMesh, VertexSemantic.BiTangent),
                TexCoords = Enumerable.Range(0, 4)
                    .Select(i => GetVertices<Vector2>(srcMesh, VertexSemantic.TexCoord, number: i))
                    .Where(arr => arr is not null)
                    .ToList()
            };

            return dstMesh;
        }
    }
}
