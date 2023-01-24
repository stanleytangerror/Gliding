﻿using System;
using System.Runtime.InteropServices;

namespace Interop
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vec2i
    {
        public Int32 x, y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Vec2u
    {
        public UInt32 x, y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Vec2f
    {
        public float x, y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Vec4f
    {
        public float x, y, z, w;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct WindowRuntimeInfo
    {
        public UInt64 mNativeHandle;
        public Vec2i mSize;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct WindowId
    {
        public UInt64 mId;
    }
}
