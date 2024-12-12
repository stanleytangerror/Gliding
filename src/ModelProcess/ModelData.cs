using System.Numerics;

namespace ModelProcess
{
    [ByteSerializable]
    public struct ChannelData
    {
        public required string Name = string.Empty;
        public string TexturePath = string.Empty;
        public int TexCoord;
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
        public required UInt16[] Indices = [];
        public required Vector3[] Positions = [];
        public Vector3[] Normals = [];
        public Vector3[] Tangents = [];
        public Vector3[] BiTangents = [];
        public IList<Vector2[]> TexCoords = [];

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
