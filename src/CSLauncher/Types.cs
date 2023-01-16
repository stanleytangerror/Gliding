using System;
using System.Runtime.InteropServices;

namespace Interop
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vec2i
    {
        public Int32 x, y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct WindowInfo
    {
        public UInt64 mWindowHandle;
        public Vec2i mSize;
    }
}
