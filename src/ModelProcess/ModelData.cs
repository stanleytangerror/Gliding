using System.Numerics;

namespace ModelProcess
{
    [ByteSerializable]
    public enum VertexSemantic : UInt16
    {
        Position, Normal, Tangent, BiTangent, TexCoord, Color, Semantic_Count
    }

    [ByteSerializable]
    public enum ScalarType : UInt16
    {
        Float, Double, Int32, UInt32, Int16, UInt16
    }

    [ByteSerializable]
    public struct VertexAttributeMeta
    {
        public VertexSemantic Semantic;
        public UInt16 SemanticIndex;
        public ScalarType ScalarType;
        public UInt16 SizeInBytes;
        public UInt16 OffsetInBytes;
    }

    [ByteSerializable]
    public struct ChannelData
    {
        public required string Name = string.Empty;
        public string TexturePath = string.Empty;
        public Int32 TexCoord;
        public Dictionary<string, float> ScalarParams = [];
        public Dictionary<string, Vector2> Vector2Params = [];
        public Dictionary<string, Vector3> Vector3Params = [];
        public Dictionary<string, Vector4> Vector4Params = [];

        public ChannelData()
        {
        }
    }

    [ByteSerializable]
    public struct MaterialData
    {
        public required Guid Id;
        public required string Name = string.Empty;
        public required ChannelData[] Channels = [];

        public MaterialData()
        {
        }
    }

    [ByteSerializable]
    public struct MeshData
    {
        public required string Name = string.Empty;
        public required Guid MaterialId;
        public required VertexAttributeMeta[] VertexAttributeMetas = [];
        public required byte[] Vertices = [];
        public required UInt16[] Indices = [];

        public MeshData()
        {
        }
    }

    [ByteSerializable]
    public struct ModelData
    {
        public required string Name = string.Empty;
        public required MaterialData[] Materials = [];
        public required MeshData[] Meshes = [];

        public ModelData()
        {
        }
    }
}
