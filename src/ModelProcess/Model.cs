using System.Numerics;
using System.Text.Json.Serialization;
using System.Text.Json;

namespace ModelProcess
{
    enum ScalarType
    {
        Float, Double, Int32, UInt32, Int16, UInt16
    }

    enum VertexSemantic
    {
        Position,
        Normal, Tangent, BiTangent,
        TexCoord, Color
    }

    struct VertexAttribute
    {
        VertexSemantic Semantic;
        uint SemanticNumber;
        ScalarType ScalarType;
        uint Dimension;
    }

    public class Mesh
    {
        public required Material? Material { get; set; }
        public Vector3[] Positions { get; set; } = [];
        public Vector3[] Normals { get; set; }    = [];
        public Vector3[] Tangents { get; set; }   = [];
        public Vector3[] BiTangents { get; set; } = [];
        public List<Vector2[]> TexCoords { get; set; } = [];
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

        public ModelData ToStorageData()
        {
            var materialToId = this.Materials.ToDictionary(m => m, m => Guid.NewGuid());

            ModelData result = new ModelData
            {
                Name = this.Name,
                Materials = this.Materials.Select(m => new MaterialData
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
                Meshes = this.Meshes.Select(m => new MeshData
                {
                    MaterialId = materialToId[m.Material],
                    Positions = m.Positions,
                    Normals = m.Normals,
                    Tangents = m.Tangents,
                    BiTangents = m.BiTangents,
                    TexCoords = m.TexCoords,
                }).ToArray(),
            };

            return result;
        }
    }
}
