using System.Numerics;
using UkooLabs.FbxSharpie.Extensions;
using FBX = UkooLabs.FbxSharpie;

namespace ModelProcess
{
    public static class FbxLoader
    {
        /* https://github.com/UkooLabs/FBXSharpie */
        public static Model Load(string path)
        {
            if (FBX.FbxIO.IsBinaryFbx(path))
            {
                throw new Exception("Binary FBX is not supported");
            }

            var document = FBX.FbxIO.Read(path);

            // load objects
            var objects = document.GetRootNodeWithName("Objects");
            var textures = objects.GetNodesWithName("Texture").Select(RawFbxTexture.LoadFbxNode).ToDictionary(t => t.Id, t => t);
            var materials = objects.GetNodesWithName("Material").Select(RawFbxMaterial.LoadFbxNode).ToDictionary(t => t.Id, t => t);
            var geometries = objects.GetNodesWithName("Geometry").Select(RawFbxGeometry.LoadFbxNode).ToDictionary(t => t.Id, t => t);
            var models = objects.GetNodesWithName("Model").Select(RawFbxModel.LoadFbxNode).ToDictionary(t => t.Id, t => t);

            // build connections
            var connections = document.GetRootNodeWithName("Connections");
            var root = BuildConnections(connections, textures, materials, geometries, models);

            var dstTextures = textures.ToDictionary(t => t.Key, t => LoadTexture(t.Value));
            var dstMaterials = materials.ToDictionary(m => m.Key, m => LoadMaterial(m.Value, t => dstTextures[t]));
            var dstMeshes = geometries.ToDictionary(g => g.Key, g => LoadMesh(g.Value, i => models[i], i => dstMaterials[i]));
            var dstMeshInstances = models.Where(m => m.Value.IsMesh && m.Value.Geometry != default)
                .Select(m => LoadMeshInstance(m.Value, i => models[i], i => dstMeshes[i]));

            return new()
            {
                Name = path,
                Textures = dstTextures.Select(t => t.Value).ToArray(),
                Materials = dstMaterials.Select(t => t.Value).ToArray(),
                Meshes = dstMeshes.Select(t => t.Value).ToArray(),
                MeshInstances = dstMeshInstances.ToArray(),
            };
        }

        static Texture LoadTexture(RawFbxTexture srcTexture)
            => new()
            {
                Path = srcTexture.Path,
                Sampler = new()
                {
                    MinFilter = TextureInterpolationFilter.Linear,
                    MagFilter = TextureInterpolationFilter.Linear,
                    MipMapFilter = TextureInterpolationFilter.Linear,
                    AddressMode = [ TextureAddressMode.Wrap, TextureAddressMode.Wrap, TextureAddressMode.Clamp ]
                },
            };

        static Material LoadMaterial(RawFbxMaterial material, Func<int, Texture> getTexture)
            => new()
            {
                Name = material.Name,
                Channels = material.TextureUsages.Select(tu => new Material.Channel
                {
                    Name = tu.usage,
                    Texture = getTexture(tu.textureId),
                }).ToArray(),
            };

        static Mesh LoadMesh(RawFbxGeometry geometry, Func<int, RawFbxModel> getFbxModel, Func<int, Material> getMaterial)
        {
            var materialId = geometry.ParentMeshes
                .Select(m => getFbxModel(m).MaterialId)
                .Where(m => m is not null)
                .Distinct().Single()!.Value;

            return new()
            {
                Name = "",
                Material = getMaterial(materialId),
                Indices = Enumerable.Range(0, geometry.Indices.Length).Select(i => (uint)i).ToArray(),
                Positions = geometry.Indices.Select(i => geometry.Positions[(i + geometry.Positions.Count()) % geometry.Positions.Count()]).ToArray(),
                Normals = geometry.Normals,
                Tangents = geometry.Tangents,
                BiTangents = geometry.BiTangents,
                TexCoords = geometry.TexCoords.Select(d => d.ToDirectData()).ToList(),
            };
        }

        static MeshInstance LoadMeshInstance(RawFbxModel model, Func<int, RawFbxModel> getFbxModel, Func<int, Mesh> getMesh)
        {
            var transform = model.LocalTransform;
            for (int modelId = model.Id; getFbxModel(modelId).ParentModel is not null && getFbxModel(modelId).ParentModel != 0; modelId = getFbxModel(modelId).ParentModel!.Value)
            {
                transform = getFbxModel(modelId).LocalTransform * transform;
            }

            return new()
            {
                Mesh = getMesh(model.Geometry!.Value),
                LocalTransform = transform,
            };
        }

        static IEnumerable<FBX.FbxNode> GetNodesWithName(this FBX.FbxNode node, string name)
        {
            if (node?.Identifier.Value == name)
            {
                yield return node;
            }
            else
            {
                foreach (var n in node?.Nodes?.SelectMany(n => GetNodesWithName(n, name)) ?? [])
                {
                    yield return n;
                }
            }
        }

        static FBX.FbxNode GetRootNodeWithName(this FBX.FbxDocument document, string name)
        {
            return document.Nodes.Single(n => n.Identifier.Value == name);
        }

        static Vector2[] ToVector2Array(this IEnumerable<float> data) => data.Chunk(2).Select(v => new Vector2(v[0], v[1])).ToArray();
        static Vector3[] ToVector3Array(this IEnumerable<float> data) => data.Chunk(3).Select(v => new Vector3(v[0], v[1], v[2])).ToArray();

        static RawFbxRoot BuildConnections(
            FBX.FbxNode connections, 
            Dictionary<int, RawFbxTexture> textures, 
            Dictionary<int, RawFbxMaterial> materials,
            Dictionary<int, RawFbxGeometry> geometries, 
            Dictionary<int, RawFbxModel> models)
        {
            RawFbxRoot root = new();

            foreach (var conn in connections.Nodes)
            {
                if (conn.Identifier.Value == "C" && conn.Properties[0].GetAsString() == "OP")
                {
                    var texId = conn.Properties[1].GetAsIntegr();
                    var matId = conn.Properties[2].GetAsIntegr();
                    var usage = conn.Properties[3].GetAsString();

                    if (materials.ContainsKey(matId) && textures.ContainsKey(texId))
                    {
                        materials[matId].AddTexture(texId, usage);
                    }
                }
                else if (conn.Identifier.Value == "C" && conn.Properties[0].GetAsString() == "OO")
                {
                    var leftId = conn.Properties[1].GetAsIntegr();
                    var rightId = conn.Properties[2].GetAsIntegr();

                    if (materials.ContainsKey(leftId) && models.ContainsKey(rightId))
                    {
                        models[rightId].MaterialId = leftId;
                    }
                    else if (geometries.ContainsKey(leftId) && models.ContainsKey(rightId))
                    {
                        models[rightId].Geometry = leftId;
                        geometries[leftId].AddParentMeshes(rightId);
                    }
                    else if (models.ContainsKey(leftId) && rightId == 0)
                    {
                        root.AddChildModel(leftId);
                        models[leftId].ParentModel = rightId;
                    }
                    else if (models.ContainsKey(leftId) && models.ContainsKey(rightId))
                    {
                        models[rightId].AddChildModel(leftId);
                        models[leftId].ParentModel = rightId;
                    }
                }
            }

            return root;
        }

        class RawFbxNodeBase
        {
            public int Id { get; init; }
        }

        class RawFbxTexture : RawFbxNodeBase
        {
            public required string Path { get; init; }

            public static RawFbxTexture LoadFbxNode(FBX.FbxNode node)
            {
                return new()
                {
                    Id = node.Properties[0].GetAsIntegr(),
                    Path = node.Nodes.Single(n => n.Identifier.Value == "FileName").Value.GetAsString(),
                };
            }
        }

        class RawFbxMaterial : RawFbxNodeBase
        {
            public required string Name { get; init; }
            public List<(int textureId, string usage)> TextureUsages { get; } = [];

            public void AddTexture(int textureId, string usage)
            {
                TextureUsages.Add((textureId, usage));
            }

            public static RawFbxMaterial LoadFbxNode(FBX.FbxNode node)
            {
                return new()
                {
                    Id = node.Properties[0].GetAsIntegr(),
                    Name = node.Properties[1].GetAsString(),
                };
            }
        }

        class RawFbxGeometry : RawFbxNodeBase
        {
            public class IndexToDirectData<T> where T : struct
            {
                /* mapped to fbx content ReferenceInformationType: "IndexToDirect" */
                public required T[] Data { get; init; } = [];
                public required int[] Indices { get; init; } = [];

                public T[] ToDirectData() => Indices.Any() ? Indices.Select(i => Data[i]).ToArray() : [];
            }

            public required Vector3[] Positions { get; set; }
            public required int[] Indices { get; set; }
            public Vector3[] Normals { get; set; } = [];
            public Vector3[] Tangents { get; set; } = [];
            public Vector3[] BiTangents { get; set; } = [];
            public IndexToDirectData<Vector2>[] TexCoords { get; set; } = [];

            public List<int> ParentMeshes { get; } = [];

            public void AddParentMeshes(int meshId)
            {
                ParentMeshes.Add(meshId);
            }

            public static RawFbxGeometry LoadFbxNode(FBX.FbxNode node)
            {
                var layerNormal = node.Nodes.Where(n => n.Identifier.Value == "LayerElementNormal");
                var layerUvMultiple = Enumerable.Range(0, 4).Select(i =>
                    node.Nodes.Where(n => n.Identifier.Value == "LayerElementUV" && n.Properties[0].GetAsIntegr() == i));

                return new()
                {
                    Id = node.Properties[0].GetAsIntegr(),

                    Indices = node.Nodes.Single(n => n.Identifier.Value == "PolygonVertexIndex").Value.GetAsIntArray(),

                    Positions = node.Nodes.Single(n => n.Identifier.Value == "Vertices").Value.GetAsFloatArray().ToVector3Array(),

                    Normals = layerNormal.Any() ?
                        layerNormal.Single()
                            .Nodes.Single(n => n.Identifier.Value == "Normals").Value.GetAsFloatArray().ToVector3Array()
                            : [],

                    TexCoords = layerUvMultiple.Select(l =>
                        l.Any() ?
                        new IndexToDirectData<Vector2>
                        {
                            Data = l.Single().Nodes.Single(n => n.Identifier.Value == "UV").Value.GetAsFloatArray().ToVector2Array(),
                            Indices = l.Single().Nodes.Single(n => n.Identifier.Value == "UVIndex").Value.GetAsIntArray()
                        } :
                        new IndexToDirectData<Vector2> { Data = [], Indices = [] }
                    ).ToArray()
                };
            }
        }

        class RawFbxModel : RawFbxNodeBase
        {
            public string Name { get; set; }
            public bool IsMesh { get; set; } = false;
            public Vector3 LocalTranslation { get; set; }
            public Vector3 LocalRotation { get; set; }
            public Vector3 LocalScaling { get; set; }

            public Matrix4x4 LocalTransform
            {
                get => MathUtils.CreateTransform(LocalTranslation, LocalRotation, LocalScaling);
            }

            private int? _materialId = default;
            public int? MaterialId
            {
                get => _materialId;
                set
                {
                    if (_materialId != default && _materialId != value)
                    {
                        throw new Exception($"Duplicate material {_materialId} and {value} of model {Id}");
                    }
                    _materialId = value;
                }
            }

            public int? _geometryId = default;
            public int? Geometry
            {
                get => _geometryId;
                set
                {
                    if (_geometryId != default && _geometryId != value)
                    {
                        throw new Exception($"Duplicate geometry {_geometryId} and {value} of model {Id}");
                    }
                    _geometryId = value;
                }
            }

            private int? _parentModel = default;
            public int? ParentModel
            {
                get => _parentModel;
                set
                {
                    if (_parentModel != default && _parentModel != value)
                    {
                        throw new Exception($"Duplicate parent model {_parentModel} and {value} of model {Id}");
                    }
                    _parentModel = value;
                }
            }

            public List<int> ChildrenModel { get; } = [];

            public void AddChildModel(int modelId)
            {
                ChildrenModel.Add(modelId);
            }

            public static RawFbxModel LoadFbxNode(FBX.FbxNode node)
            {
                var properties = node.Nodes.Single(n => n.Identifier.Value == "Properties70");

                var locTransNode = properties.Nodes.Where(n => n.Identifier.Value == "P" && n.Properties[0].GetAsString() == "Lcl Translation");
                Vector3 locTrans = locTransNode.Any() ?
                    new(locTransNode.Single().Properties[4].GetAsFloat(), locTransNode.Single().Properties[5].GetAsFloat(), locTransNode.Single().Properties[6].GetAsFloat())
                    : Vector3.One;

                var locRotNode = properties.Nodes.Where(n => n.Identifier.Value == "P" && n.Properties[0].GetAsString() == "Lcl Rotation");
                Vector3 locRot = locRotNode.Any() ?
                    new(locRotNode.Single().Properties[4].GetAsFloat(), locRotNode.Single().Properties[5].GetAsFloat(), locRotNode.Single().Properties[6].GetAsFloat())
                    : Vector3.One;

                var locScaleNode = properties.Nodes.Where(n => n.Identifier.Value == "P" && n.Properties[0].GetAsString() == "Lcl Scaling");
                Vector3 locScale = locScaleNode.Any() ?
                    new(locScaleNode.Single().Properties[4].GetAsFloat(), locScaleNode.Single().Properties[5].GetAsFloat(), locScaleNode.Single().Properties[6].GetAsFloat())
                    : Vector3.One;
                
                return new()
                {
                    Id = node.Properties[0].GetAsIntegr(),
                    Name = node.Properties[1].GetAsString(),
                    IsMesh = node.Properties[2].GetAsString() != "Null",
                    LocalTranslation = locTrans,
                    LocalRotation = locRot,
                    LocalScaling = locScale
                };
            }
        }

        class RawFbxRoot : RawFbxNodeBase
        {
            public List<int> ChildrenModel { get; } = [];

            public RawFbxRoot()
            {
                Id = 0;
            }

            public void AddChildModel(int modelId)
            {
                ChildrenModel.Add(modelId);
            }
        }
    }
}
