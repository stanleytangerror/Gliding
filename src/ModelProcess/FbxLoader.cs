using System.Diagnostics.CodeAnalysis;
using System.Numerics;
using UkooLabs.FbxSharpie.Extensions;
using FBX = UkooLabs.FbxSharpie;

namespace ModelProcess
{
    public static class FbxLoader
    {
        /* https://github.com/UkooLabs/FBXSharpie
         */
        public static Model Load(string path)
        {
            if (FBX.FbxIO.IsBinaryFbx(path))
            {
                throw new Exception("Binary FBX is not supported");
            }

            var document = FBX.FbxIO.Read(path);

            var objects = document.GetRootNodeWithName("Objects");
            var connections = document.GetRootNodeWithName("Connections");

            var texIdToDstTexs = objects.GetNodesWithName("Texture").ToDictionary(t => t.GetNodeId(), t => LoadTexture(t));
            var matTexUsage = LoadTextureUsages(connections, t => texIdToDstTexs.TryGetValue(t, out var v) ? v : null);
            var matIdToDstMats = objects.GetNodesWithName("Material").ToDictionary(m => m.GetNodeId(), m => LoadMaterial(m, matTexUsage[m.GetNodeId()]));
            var geoIdToDstMesh = objects.GetNodesWithName("Geometry").ToDictionary(g => g.GetNodeId(), g => LoadGeometry(g, m => matIdToDstMats[m]));

            Model dstModel = new()
            {
                Name = path,
                Materials = matIdToDstMats.Select(p => p.Value).ToArray(),
                Meshes = geoIdToDstMesh.Select(p => p.Value).ToArray()
            };

            return dstModel;

        }

        static int GetNodeId(this FBX.FbxNode node)
            => node.Properties[0].GetAsIntegr();

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

        static FBX.FbxNode GetNodeWithId(this FBX.FbxNode node, int id)
        {
            if (Inner(node, id, out FBX.FbxNode? result))
            {
                return result;
            }

            throw new Exception($"No FbxNode with id {id}");

            static bool Inner(FBX.FbxNode node, int id, [NotNullWhen(true)] out FBX.FbxNode? result)
            {
                if (node?.GetNodeId() == id)
                {
                    result = node;
                    return true;
                }

                foreach (var n in node?.Nodes?.Where(n => n is not null) ?? [])
                {
                    if (Inner(n, id, out result))
                    {
                        return true;
                    }
                }

                result = null;
                return false;
            }
        }

        static Dictionary<int, (Texture texture, string usage)[]> LoadTextureUsages(FBX.FbxNode connections, Func<int, Texture?> getTexture)
        {
            return connections.Nodes
                .Where(c => c.Identifier.Value == "C" && c.Properties[0].GetAsString() == "OP")
                .Select(c => (
                    c.Properties[2].GetAsIntegr(), // material id
                    getTexture(c.Properties[1].GetAsIntegr()), // texture
                    c.Properties[3].GetAsString()  // usage
                ))
                .Where(c => c.Item2 is not null)
                .GroupBy(p => p.Item1, p => (p.Item2!, p.Item3))
                .ToDictionary(p => p.Key, p => p.ToArray());
        }

        static Texture LoadTexture(FBX.FbxNode srcTexture)
        {
            return new Texture
            {
                Path = srcTexture.Nodes.Single(n => n.Identifier.Value == "FileName").Value.GetAsString(),
            };
        }

        static Material LoadMaterial(FBX.FbxNode node, (Texture texture, string usage)[] textureUsages)
        {
            return new Material()
            {
                Name = node.Properties[1].GetAsString(),
                Channels = textureUsages.Select(tu => new Material.Channel
                {
                    Name = tu.usage,
                    Texture = tu.texture,
                }).ToArray(),
            };
        }

        static Mesh LoadGeometry(FBX.FbxNode node, Func<int, Material> getDstMaterial)
        {
            var id = node.GetNodeId();

            // read raw data
            var indices = node.Nodes.Single(n => n.Identifier.Value == "PolygonVertexIndex").Value.GetAsIntArray();
            
            var positions = node.Nodes.Single(n => n.Identifier.Value == "Vertices").Value.GetAsDoubleArray().Select(v => (float)v)
                .Chunk(3).Select(v => new Vector3(v[0], v[1], v[2])).ToArray();

            var normalData = node.Nodes.Where(n => n.Identifier.Value == "LayerElementNormal");
            if (normalData.Any())
            {
                var normals = normalData.Single()
                    .Nodes.Single(n => n.Identifier.Value == "Normals").Value.GetAsDoubleArray().Select(v => (float)v)
                    .Chunk(3).Select(v => new Vector3(v[0], v[1], v[2])).ToArray();
            }

            foreach (var i in Enumerable.Range(0, 4))
            {
                var texCoordData = node.Nodes.Where(n => n.Identifier.Value == "LayerElementUV" && n.Properties[0].GetAsIntegr() == i);
                if (texCoordData.Any())
                {
                    var uv = texCoordData.Single()
                        .Nodes.Single(n => n.Identifier.Value == "UV").Value.GetAsDoubleArray().Select(v => (float)v)
                    .Chunk(2).Select(v => new Vector2(v[0], v[1])).ToArray();
                    var uvIndex = texCoordData.Single()
                        .Nodes.Single(n => n.Identifier.Value == "UVIndex").Value.GetAsIntArray()
                        .Chunk(2).Select(v => new Vector2(v[0], v[1])).ToArray();
                }
            }

            var materialId = node.Nodes.Single(n => n.Identifier.Value == "LayerElementMaterial").Value.GetAsIntArray().Single();

            // post process
            var finalIndices = indices.Select(i => (uint)(i % positions.Length));

            return new Mesh()
            {
                Name = "",
                Material = getDstMaterial(materialId),
            };
        }
    }
}
