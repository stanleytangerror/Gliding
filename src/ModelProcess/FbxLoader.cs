using System.Diagnostics.CodeAnalysis;
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

            var matTexUsage = LoadConnections(connections);
            var texIdToDstTexs = objects.GetNodesWithName("Texture").ToDictionary(t => t.GetNodeId(), t => LoadTexture(t));
            var matIdToDstMats = objects.GetNodesWithName("Material").ToDictionary(m => m.GetNodeId(), m => LoadMaterial(m));
            var geoIdToDstMesh = objects.GetNodesWithName("Geometry").ToDictionary(g => g.GetNodeId(), g => LoadGeometry(document, g.GetNodeId(), m => matIdToDstMats[m]));

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

        static IEnumerable<(int texId, int matId, string usage)> LoadConnections(FBX.FbxNode connections)
        {
            return connections.Nodes
                .Where(c => c.Identifier.Value == "C" && c.Properties[0].GetAsString() == "OP")
                .Select(c => (
                    c.Properties[1].GetAsIntegr(),
                    c.Properties[2].GetAsIntegr(),
                    c.Properties[3].GetAsString()
                ));
        }

        static Texture LoadTexture(FBX.FbxNode srcTexture)
        {
            return new Texture
            {
                Path = srcTexture.Nodes.Single(n => n.Identifier.Value == "FileName").Value.GetAsString(),
            };
        }

        static Material LoadMaterial(FBX.FbxNode node)
        {
            return new Material()
            {
                Name = node.Properties[1].GetAsString()
            };
        }

        static Mesh LoadGeometry(FBX.FbxDocument document, int geometryId, Func<int, Material> getDstMaterial)
        {
            var vertexIndices = document.GetVertexIndices(geometryId);

            var materials = document.GetGeometryHasMaterials(geometryId) ?
                    document.GetMaterials(geometryId, vertexIndices, document.GetLayerIndices(geometryId, FBX.FbxLayerElementType.Material)[0]).ToHashSet() : [];

            if (materials.Count != 1)
            {
                throw new Exception($"Invalid material count {materials.Count} for geometry id {geometryId}");
            }

            return new Mesh()
            {
                Name = "",
                Material = getDstMaterial(materials.First()),
                Indices = vertexIndices.Select(i => (uint)i).ToArray(),
                Positions = document.GetPositions(geometryId, vertexIndices) ?? [],
                Normals = document.GetGeometryHasNormals(geometryId) ?
                    document.GetNormals(geometryId, vertexIndices, document.GetLayerIndices(geometryId, FBX.FbxLayerElementType.Normal)[0]) : [],
                Tangents = document.GetGeometryHasTangents(geometryId) ?
                    document.GetTangents(geometryId, vertexIndices, document.GetLayerIndices(geometryId, FBX.FbxLayerElementType.Tangent)[0]) : [],
                BiTangents = document.GetGeometryHasBinormals(geometryId) ?
                    document.GetBinormals(geometryId, vertexIndices, document.GetLayerIndices(geometryId, FBX.FbxLayerElementType.Binormal)[0]) : [],
                TexCoords = Enumerable.Range(0, 4)
                    .Select(i =>
                        document.GetGeometryHasTexCoords(geometryId) ?
                        document.GetTexCoords(geometryId, vertexIndices, document.GetLayerIndices(geometryId, FBX.FbxLayerElementType.TexCoord)[i]) : [])
                    .Where(d => d.Length > 0)
                    .ToList(),
            };
        }
    }
}
