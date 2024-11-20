using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace ModelProcess
{
    public struct ModelData
    {

        public struct ChannelData
        {
            public string Name;
            public string TexturePath;
            public int TexCoord;
            public IDictionary<string, float> ScalarParams;
            public IDictionary<string, Vector2> Vector2Params;
            public IDictionary<string, Vector3> Vector3Params;
            public IDictionary<string, Vector4> Vector4Params;
        }
    }

    public struct MaterialData
    {

    }
}
