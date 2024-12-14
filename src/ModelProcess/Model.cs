using System.Numerics;

namespace ModelProcess
{
    public class Mesh
    {
        public required string Name { get; set; } = string.Empty;
        public required Material? Material { get; set; }
        public uint[] Indices { get; set; } = [];
        public Vector3[] Positions { get; set; } = [];
        public Vector3[] Normals { get; set; }    = [];
        public Vector3[] Tangents { get; set; }   = [];
        public Vector3[] BiTangents { get; set; } = [];
        public List<Vector2[]> TexCoords { get; set; } = [];
        public Vector4[] Colors { get; set; } = [];
    }

    public class Material
    {
        public string Name { get; set; } = string.Empty;
        public Channel[] Channels { get; set; } = [];

        public class Channel
        { 
            public string Name { get; set; } = string.Empty;
            public string TexturePath { get; set; } = string.Empty;
            public int TexCoord { get; set; }
            public Dictionary<string, float> ScalarParams { get; set; } = [];
            public Dictionary<string, Vector2> Vector2Params { get; set; } = [];
            public Dictionary<string, Vector3> Vector3Params { get; set; } = [];
            public Dictionary<string, Vector4> Vector4Params { get; set; } = [];
        }
    }

    public class Model
    {
        public string Name { get; set; } = string.Empty;
        public Material[] Materials { get; set; } = [];
        public Mesh[] Meshes { get; set; } = [];
    }

    public static class ModelExtensions
    {
        public static VertexAttributeMeta[] ToVertexAttributeMetas(this Mesh mesh)
        {
            List<VertexAttributeMeta> result = [];
            UInt16 offset = 0;
            if (mesh.Positions?.Length > 0)
            {
                result.Add(new VertexAttributeMeta
                {
                    Semantic = VertexSemantic.Position,
                    SemanticIndex = 0,
                    ScalarType = ScalarType.Float,
                    SizeInBytes = 12,
                    OffsetInBytes = offset,
                });
                offset += result.Last().SizeInBytes;
            }
            if (mesh.Normals?.Length > 0)
            {
                result.Add(new VertexAttributeMeta
                {
                    Semantic = VertexSemantic.Normal,
                    SemanticIndex = 0,
                    ScalarType = ScalarType.Float,
                    SizeInBytes = 12,
                    OffsetInBytes = offset,
                });
                offset += result.Last().SizeInBytes;
            }
            if (mesh.Tangents?.Length > 0)
            {
                result.Add(new VertexAttributeMeta
                {
                    Semantic = VertexSemantic.Tangent,
                    SemanticIndex = 0,
                    ScalarType = ScalarType.Float,
                    SizeInBytes = 12,
                    OffsetInBytes = offset,
                });
                offset += result.Last().SizeInBytes;
            }
            if (mesh.BiTangents?.Length > 0)
            {
                result.Add(new VertexAttributeMeta
                {
                    Semantic = VertexSemantic.BiTangent,
                    SemanticIndex = 0,
                    ScalarType = ScalarType.Float,
                    SizeInBytes = 12,
                    OffsetInBytes = offset,
                });
                offset += result.Last().SizeInBytes;
            }
            foreach (var (TexCoord, index) in mesh
                .TexCoords.Select((channel, index) => (channel, index))
                .Where((channel, index) => channel.channel.Length > 0))
            {
                result.Add(new VertexAttributeMeta
                {
                    Semantic = VertexSemantic.TexCoord,
                    SemanticIndex = (UInt16)index,
                    ScalarType = ScalarType.Float,
                    SizeInBytes = 8,
                    OffsetInBytes = offset,
                });
                offset += result.Last().SizeInBytes;
            }
            if (mesh.Colors?.Length > 0)
            {
                result.Add(new VertexAttributeMeta
                {
                    Semantic = VertexSemantic.Color,
                    SemanticIndex = 0,
                    ScalarType = ScalarType.Float,
                    SizeInBytes = 16,
                });
                offset += result.Last().SizeInBytes;
            }
            return result.ToArray();
        }

        public static byte[] ToVerticesData(this Mesh mesh)
        {
            var vertexCount = mesh.Positions.Length;

            var resultSize = mesh.ToVertexAttributeMetas().Sum(m => m.SizeInBytes) * vertexCount;

            byte[] result = new byte[resultSize];

            using var stream = new MemoryStream(result);
            using CustomedBinaryWriter writer = new(stream);

            foreach (var i in Enumerable.Range(0, vertexCount))
            {
                if (mesh.Positions.Length > 0) { writer.Serialize(mesh.Positions[i]); }
                if (mesh.Normals.Length > 0) { writer.Serialize(mesh.Normals[i]); }
                if (mesh.Tangents.Length > 0) { writer.Serialize(mesh.Tangents[i]); }
                if (mesh.BiTangents.Length > 0) { writer.Serialize(mesh.BiTangents[i]); }
                foreach (var texCoord in mesh.TexCoords)
                {
                    if (texCoord.Length > 0) { writer.Serialize(texCoord[i]); }
                }
                if (mesh.Colors.Length > 0) { writer.Serialize(mesh.Colors[i]); }
            }

            return result;
        }

        public static ModelData ToStorageData(this Model model)
        {
            var materialToId = model.Materials.ToDictionary(m => m, m => Guid.NewGuid());

            ModelData result = new ModelData
            {
                Name = model.Name,
                Materials = model.Materials.Select(m => new MaterialData
                {
                    Id = materialToId[m],
                    Name = m.Name,
                    Channels = m.Channels.Select(c => new ChannelData
                    {
                        Name = c.Name,
                        TexturePath = c.TexturePath,
                        TexCoord = c.TexCoord,
                        ScalarParams = c.ScalarParams,
                        Vector2Params = c.Vector2Params,
                        Vector3Params = c.Vector3Params,
                        Vector4Params = c.Vector4Params,
                    }).ToArray(),
                }).ToArray(),
                Meshes = model.Meshes.Select(m => new MeshData
                {
                    Name = m.Name,
                    MaterialId = materialToId[m.Material],
                    Indices = m.Indices.Select(i => (UInt16)i).ToArray(),
                    VertexAttributeMetas = m.ToVertexAttributeMetas(),
                    Vertices = m.ToVerticesData(),
                }).ToArray(),
            };

            return result;
        }
    }
}
