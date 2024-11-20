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
        public Material? Material { get; set; }
        public Vector3[]? Positions { get; set; }
        public Vector3[]? Normals { get; set; }
        public Vector3[]? Tangents { get; set; }
        public Vector3[]? BiTangents { get; set; }
        public IList<Vector2[]>? TexCoords { get; set; }
    }

    public class Material
    {
        public string Name { get; set; }
        public Channel[] Channels { get; set; }

        public class Channel
        { 
            public string Name { get; set; }
            public string TexturePath { get; set; }
            public int TexCoord { get; set; }
            public IDictionary<string, float> ScalarParams { get; set; }
            public IDictionary<string, Vector2> Vector2Params { get; set; }
            public IDictionary<string, Vector3> Vector3Params { get; set; }
            public IDictionary<string, Vector4> Vector4Params { get; set; }
        }
    }

    public class Model
    {
        public string? Name { get; set; }
        public Material[]? Materials { get; set; }
        public Mesh[]? Meshes { get; set; }
    }
}
