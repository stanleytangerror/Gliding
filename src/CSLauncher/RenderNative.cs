using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace Interp
{
    class RenderNative
    {
        [StructLayout(LayoutKind.Sequential)]
        struct Vec2i 
        {
            public Int32 x, y;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct WindowInfo
        {
            public UInt64 mWindowHandle;
            public Vec2i mSize;
        }

        [DllImport("../Render_Debug_x64.dll")]
        extern static IntPtr CreateRenderModule();
    }
}
