#pragma once

namespace RHI
{
    enum class InputClassification
    {
        PER_VERTEX_DATA = 0,
        PER_INSTANCE_DATA = 1
    };

    enum class ResourceDimention
    {
        Unknown = 0,
        Buffer = 1,
        Texture1D = 2,
        Texture2D = 3,
        Texture3D = 4
    };

    enum class ViewDimension
    {
        UNKNOWN = 0,
        BUFFER = 1,
        TEXTURE1D = 2,
        TEXTURE1DARRAY = 3,
        TEXTURE2D = 4,
        TEXTURE2DARRAY = 5,
        TEXTURE2DMS = 6,
        TEXTURE2DMSARRAY = 7,
        TEXTURE3D = 8,
        TEXTURECUBE = 9,
        TEXTURECUBEARRAY = 10,
        RAYTRACING_ACCELERATION_STRUCTURE = 11
    };

    enum class PixelFormat : u32
    {
        UNKNOWN = 0,
        R32G32B32A32_TYPELESS = 1,
        R32G32B32A32_FLOAT = 2,
        R32G32B32A32_UINT = 3,
        R32G32B32A32_SINT = 4,
        R32G32B32_TYPELESS = 5,
        R32G32B32_FLOAT = 6,
        R32G32B32_UINT = 7,
        R32G32B32_SINT = 8,
        R16G16B16A16_TYPELESS = 9,
        R16G16B16A16_FLOAT = 10,
        R16G16B16A16_UNORM = 11,
        R16G16B16A16_UINT = 12,
        R16G16B16A16_SNORM = 13,
        R16G16B16A16_SINT = 14,
        R32G32_TYPELESS = 15,
        R32G32_FLOAT = 16,
        R32G32_UINT = 17,
        R32G32_SINT = 18,
        R32G8X24_TYPELESS = 19,
        D32_FLOAT_S8X24_UINT = 20,
        R32_FLOAT_X8X24_TYPELESS = 21,
        X32_TYPELESS_G8X24_UINT = 22,
        R10G10B10A2_TYPELESS = 23,
        R10G10B10A2_UNORM = 24,
        R10G10B10A2_UINT = 25,
        R11G11B10_FLOAT = 26,
        R8G8B8A8_TYPELESS = 27,
        R8G8B8A8_UNORM = 28,
        R8G8B8A8_UNORM_SRGB = 29,
        R8G8B8A8_UINT = 30,
        R8G8B8A8_SNORM = 31,
        R8G8B8A8_SINT = 32,
        R16G16_TYPELESS = 33,
        R16G16_FLOAT = 34,
        R16G16_UNORM = 35,
        R16G16_UINT = 36,
        R16G16_SNORM = 37,
        R16G16_SINT = 38,
        R32_TYPELESS = 39,
        D32_FLOAT = 40,
        R32_FLOAT = 41,
        R32_UINT = 42,
        R32_SINT = 43,
        R24G8_TYPELESS = 44,
        D24_UNORM_S8_UINT = 45,
        R24_UNORM_X8_TYPELESS = 46,
        X24_TYPELESS_G8_UINT = 47,
        R8G8_TYPELESS = 48,
        R8G8_UNORM = 49,
        R8G8_UINT = 50,
        R8G8_SNORM = 51,
        R8G8_SINT = 52,
        R16_TYPELESS = 53,
        R16_FLOAT = 54,
        D16_UNORM = 55,
        R16_UNORM = 56,
        R16_UINT = 57,
        R16_SNORM = 58,
        R16_SINT = 59,
        R8_TYPELESS = 60,
        R8_UNORM = 61,
        R8_UINT = 62,
        R8_SNORM = 63,
        R8_SINT = 64,
        A8_UNORM = 65,
        R1_UNORM = 66,
        R9G9B9E5_SHAREDEXP = 67,
        R8G8_B8G8_UNORM = 68,
        G8R8_G8B8_UNORM = 69,
        BC1_TYPELESS = 70,
        BC1_UNORM = 71,
        BC1_UNORM_SRGB = 72,
        BC2_TYPELESS = 73,
        BC2_UNORM = 74,
        BC2_UNORM_SRGB = 75,
        BC3_TYPELESS = 76,
        BC3_UNORM = 77,
        BC3_UNORM_SRGB = 78,
        BC4_TYPELESS = 79,
        BC4_UNORM = 80,
        BC4_SNORM = 81,
        BC5_TYPELESS = 82,
        BC5_UNORM = 83,
        BC5_SNORM = 84,
        B5G6R5_UNORM = 85,
        B5G5R5A1_UNORM = 86,
        B8G8R8A8_UNORM = 87,
        B8G8R8X8_UNORM = 88,
        R10G10B10_XR_BIAS_A2_UNORM = 89,
        B8G8R8A8_TYPELESS = 90,
        B8G8R8A8_UNORM_SRGB = 91,
        B8G8R8X8_TYPELESS = 92,
        B8G8R8X8_UNORM_SRGB = 93,
        BC6H_TYPELESS = 94,
        BC6H_UF16 = 95,
        BC6H_SF16 = 96,
        BC7_TYPELESS = 97,
        BC7_UNORM = 98,
        BC7_UNORM_SRGB = 99,
        AYUV = 100,
        Y410 = 101,
        Y416 = 102,
        NV12 = 103,
        P010 = 104,
        P016 = 105,
        F420_OPAQUE = 106,
        YUY2 = 107,
        Y210 = 108,
        Y216 = 109,
        NV11 = 110,
        AI44 = 111,
        IA44 = 112,
        P8 = 113,
        A8P8 = 114,
        B4G4R4A4_UNORM = 115,

        P208 = 130,
        V208 = 131,
        V408 = 132,


        SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
        SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,


        FORCE_UINT = 0xffffffff
    };

    enum class ResourceFlags
    {
        NONE = 0,
        ALLOW_RENDER_TARGET = 0x1,
        ALLOW_DEPTH_STENCIL = 0x2,
        ALLOW_UNORDERED_ACCESS = 0x4,
        DENY_SHADER_RESOURCE = 0x8,
        ALLOW_CROSS_ADAPTER = 0x10,
        ALLOW_SIMULTANEOUS_ACCESS = 0x20,
        VIDEO_DECODE_REFERENCE_ONLY = 0x40,
        VIDEO_ENCODE_REFERENCE_ONLY = 0x80
    };

    enum class FilterType
    {
		MIN_MAG_MIP_POINT = 0,
		MIN_MAG_POINT_MIP_LINEAR = 0x1,
		MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
		MIN_POINT_MAG_MIP_LINEAR = 0x5,
		MIN_LINEAR_MAG_MIP_POINT = 0x10,
		MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
		MIN_MAG_LINEAR_MIP_POINT = 0x14,
		MIN_MAG_MIP_LINEAR = 0x15,
		ANISOTROPIC = 0x55,
		COMPARISON_MIN_MAG_MIP_POINT = 0x80,
		COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
		COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
		COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
		COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
		COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
		COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
		COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
		COMPARISON_ANISOTROPIC = 0xd5,
		MINIMUM_MIN_MAG_MIP_POINT = 0x100,
		MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101,
		MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104,
		MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105,
		MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110,
		MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111,
		MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114,
		MINIMUM_MIN_MAG_MIP_LINEAR = 0x115,
		MINIMUM_ANISOTROPIC = 0x155,
		MAXIMUM_MIN_MAG_MIP_POINT = 0x180,
		MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181,
		MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184,
		MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185,
		MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190,
		MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191,
		MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194,
		MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195,
		MAXIMUM_ANISOTROPIC = 0x1d5
    };

    enum class TextureAddressMode
    {
		WRAP = 1,
		MIRROR = 2,
		CLAMP = 3,
		BORDER = 4,
		MIRROR_ONCE = 5
    };

    enum class ComparisonFunc
    {
		NEVER = 1,
		LESS = 2,
		EQUAL = 3,
		LESS_EQUAL = 4,
		GREATER = 5,
		NOT_EQUAL = 6,
		GREATER_EQUAL = 7,
		ALWAYS = 8
    };

    struct ViewPort
    {
        Vec2f LeftTop;
        Vec2f Size;
        Vec2f DepthRange;
    };

    struct Rect
    {
        Vec2f LeftTop;
        Vec2f RightBottom;
    };

    struct ProgramMacro
    {
        std::string Name;
        std::string Definition;
    };

    struct ResourceSize
    {
        u64 Width;
        u32 Height;
        u16 DepthOrArraySize;
    };

    struct CommitedResourceDesc
    {
        ResourceDimention Dimension;
        PixelFormat Format;
        ResourceSize Size;
        u16 MipLevels;
        u32 Flags; // ResourceFlags
    };

    namespace SRV
    {
        struct BufferViewDesc
        {
            u64 FirstElement;
            u32 NumElements;
            u32 StructureByteStride;
            enum Flag { None = 0, Raw = 1 } Flags;
        };

        struct Texture1DView
        {
            u32 MostDetailedMip;
            u32 MipLevels;
            f32 MinLODClamp;
        };

        struct ArrayView
        {
            u32 FirstArraySlice;
            u32 ArraySize;
        };

        struct Texture1DArrayView : Texture1DView, ArrayView {};

        struct Texture2DView : Texture1DView
        {
            u32 PlaneSlice;
        };

        struct Texture2DArrayView : Texture2DView, ArrayView {};

        struct Texture3DView : Texture1DView {};

        struct TextureCubeView : Texture1DView {};

        struct TextureCubeArrayView : TextureCubeView
        {
            u32 First2DArrayFace;
            u32 NumCubes;
        };

        struct Texture2DMSView {};

        struct Texture2DMSArrayView : ArrayView {};

        struct RaytracingAccelerationStructureView
        {
            u64 GpuVirtualAddress;
        };
    }

    namespace RTV
    {
        struct BufferViewDesc
        {
            u64 FirstElement;
            u32 NumElements;
        };

        struct Texture1DView
        {
            u32 MipSlice;
        };

        struct ArrayView
        {
            u32 FirstArraySlice;
            u32 ArraySize;
        };

        struct Texture1DArrayView : Texture1DView, ArrayView {};

        struct Texture2DView : Texture1DView
        {
            u32 PlaneSlice;
        };

        struct Texture2DArrayView : Texture2DView, ArrayView {};

        struct Texture3DView : Texture1DView 
        {
            u32 FirstWSlice;
            u32 WSize;
        };

        struct Texture2DMSView {};

        struct Texture2DMSArrayView : ArrayView {};
    }

    struct ShaderResourceViewDesc
    {
        ViewDimension Dimension;
        PixelFormat Format;
        union
        {
            SRV::BufferViewDesc      Buffer							;
            SRV::Texture1DView       Texture1D						;
            SRV::Texture1DArrayView  Texture1DArray					;
            SRV::Texture2DView       Texture2D						;
            SRV::Texture2DArrayView  Texture2DArray					;
            SRV::Texture2DMSView     Texture2DMS					   ;
            SRV::Texture2DMSArrayView  Texture2DMSArray				;
            SRV::Texture3DView       Texture3D						;
            SRV::TextureCubeView     TextureCube					   ;
            SRV::TextureCubeArrayView  TextureCubeArray				;
            SRV::RaytracingAccelerationStructureView RaytracingAccelerationStructure;
        };
    };

    struct RenderTargetViewDesc
    {
        ViewDimension Dimension;
        PixelFormat Format;
        union
        {
            RTV::BufferViewDesc      Buffer							;
            RTV::Texture1DView       Texture1D						;
            RTV::Texture1DArrayView  Texture1DArray					;
            RTV::Texture2DView       Texture2D						;
            RTV::Texture2DArrayView  Texture2DArray					;
            RTV::Texture2DMSView     Texture2DMS					   ;
            RTV::Texture2DMSArrayView  Texture2DMSArray				;
            RTV::Texture3DView       Texture3D						;
        };
    };


    struct VertexBufferViewDesc
    {
        u64 Offset;
        u32 Size;
        u32 Stride;
    };


    struct IndexBufferViewDesc
    {
        u64 Offset;
        u32 Size;
        PixelFormat Format;
    };


    struct IndexedInstancedParam
    {
        u32 IndexCount;
        u32 InstanceCount;
    };

    struct InputElementDesc
    {
        std::string SemanticName;
        u32 SemanticIndex;
        PixelFormat Format;
        u32 Slot;
        u32 AlignedByteOffset;
        InputClassification InputSlotClass;
        u32 InstanceDataStepRate;
    };

    struct SamplerDesc
    {
        FilterType Filter;
		TextureAddressMode AddressU;
		TextureAddressMode AddressV;
		TextureAddressMode AddressW;
        ComparisonFunc CompFunc;
        f32 MipLODBias;
        u32 MaxAnisotropy;
        Vec4f BorderColor;
        Vec2f LODRange;
    };
}