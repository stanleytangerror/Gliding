#pragma once

#include "AssertUtils.h"
#include "Math.h"
#include "Texture.h"

#define CAT2(X,Y) X##Y
#define CAT(X,Y) CAT2(X,Y)

#define SETTER(Type, Name) \
	protected:	Type m##Name = {}; \
	public:		using CAT(Temp, __LINE__) = Type; \
				void Set##Name(const CAT(Temp, __LINE__) & Name) { m##Name = Name; }
#define CONTINOUS_SETTER(Class, Type, Name) \
	protected:	Type m##Name = {}; \
	public:		using CAT(Temp, __LINE__) = Type; \
				Class& Set##Name(const CAT(Temp, __LINE__) & Name) { m##Name = Name; return *this; } \
                CAT(Temp, __LINE__) Get##Name() const { return m##Name; }
#define CONTINOUS_SETTER_VALUE(Class, Type, Name, DefValue)	\
	protected:	Type m##Name = (DefValue); \
	public:		using CAT(Temp, __LINE__) = Type; \
				Class& Set##Name(const CAT(Temp, __LINE__) & Name) { m##Name = Name; return *this;  } \
                CAT(Temp, __LINE__) Get##Name() const { return m##Name; }

namespace GI 
{
    constexpr u32 GetDataPitchAlignment() { return 256; }

    struct GD_COMMON_API Format
    {
        enum GD_COMMON_API Enum
        {
            FORMAT_UNKNOWN = 0,
            FORMAT_R32G32B32A32_TYPELESS = 1,
            FORMAT_R32G32B32A32_FLOAT = 2,
            FORMAT_R32G32B32A32_UINT = 3,
            FORMAT_R32G32B32A32_SINT = 4,
            FORMAT_R32G32B32_TYPELESS = 5,
            FORMAT_R32G32B32_FLOAT = 6,
            FORMAT_R32G32B32_UINT = 7,
            FORMAT_R32G32B32_SINT = 8,
            FORMAT_R16G16B16A16_TYPELESS = 9,
            FORMAT_R16G16B16A16_FLOAT = 10,
            FORMAT_R16G16B16A16_UNORM = 11,
            FORMAT_R16G16B16A16_UINT = 12,
            FORMAT_R16G16B16A16_SNORM = 13,
            FORMAT_R16G16B16A16_SINT = 14,
            FORMAT_R32G32_TYPELESS = 15,
            FORMAT_R32G32_FLOAT = 16,
            FORMAT_R32G32_UINT = 17,
            FORMAT_R32G32_SINT = 18,
            FORMAT_R32G8X24_TYPELESS = 19,
            FORMAT_D32_FLOAT_S8X24_UINT = 20,
            FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
            FORMAT_X32_TYPELESS_G8X24_UINT = 22,
            FORMAT_R10G10B10A2_TYPELESS = 23,
            FORMAT_R10G10B10A2_UNORM = 24,
            FORMAT_R10G10B10A2_UINT = 25,
            FORMAT_R11G11B10_FLOAT = 26,
            FORMAT_R8G8B8A8_TYPELESS = 27,
            FORMAT_R8G8B8A8_UNORM = 28,
            FORMAT_R8G8B8A8_UNORM_SRGB = 29,
            FORMAT_R8G8B8A8_UINT = 30,
            FORMAT_R8G8B8A8_SNORM = 31,
            FORMAT_R8G8B8A8_SINT = 32,
            FORMAT_R16G16_TYPELESS = 33,
            FORMAT_R16G16_FLOAT = 34,
            FORMAT_R16G16_UNORM = 35,
            FORMAT_R16G16_UINT = 36,
            FORMAT_R16G16_SNORM = 37,
            FORMAT_R16G16_SINT = 38,
            FORMAT_R32_TYPELESS = 39,
            FORMAT_D32_FLOAT = 40,
            FORMAT_R32_FLOAT = 41,
            FORMAT_R32_UINT = 42,
            FORMAT_R32_SINT = 43,
            FORMAT_R24G8_TYPELESS = 44,
            FORMAT_D24_UNORM_S8_UINT = 45,
            FORMAT_R24_UNORM_X8_TYPELESS = 46,
            FORMAT_X24_TYPELESS_G8_UINT = 47,
            FORMAT_R8G8_TYPELESS = 48,
            FORMAT_R8G8_UNORM = 49,
            FORMAT_R8G8_UINT = 50,
            FORMAT_R8G8_SNORM = 51,
            FORMAT_R8G8_SINT = 52,
            FORMAT_R16_TYPELESS = 53,
            FORMAT_R16_FLOAT = 54,
            FORMAT_D16_UNORM = 55,
            FORMAT_R16_UNORM = 56,
            FORMAT_R16_UINT = 57,
            FORMAT_R16_SNORM = 58,
            FORMAT_R16_SINT = 59,
            FORMAT_R8_TYPELESS = 60,
            FORMAT_R8_UNORM = 61,
            FORMAT_R8_UINT = 62,
            FORMAT_R8_SNORM = 63,
            FORMAT_R8_SINT = 64,
            FORMAT_A8_UNORM = 65,
            FORMAT_R1_UNORM = 66,
            FORMAT_R9G9B9E5_SHAREDEXP = 67,
            FORMAT_R8G8_B8G8_UNORM = 68,
            FORMAT_G8R8_G8B8_UNORM = 69,
            FORMAT_BC1_TYPELESS = 70,
            FORMAT_BC1_UNORM = 71,
            FORMAT_BC1_UNORM_SRGB = 72,
            FORMAT_BC2_TYPELESS = 73,
            FORMAT_BC2_UNORM = 74,
            FORMAT_BC2_UNORM_SRGB = 75,
            FORMAT_BC3_TYPELESS = 76,
            FORMAT_BC3_UNORM = 77,
            FORMAT_BC3_UNORM_SRGB = 78,
            FORMAT_BC4_TYPELESS = 79,
            FORMAT_BC4_UNORM = 80,
            FORMAT_BC4_SNORM = 81,
            FORMAT_BC5_TYPELESS = 82,
            FORMAT_BC5_UNORM = 83,
            FORMAT_BC5_SNORM = 84,
            FORMAT_B5G6R5_UNORM = 85,
            FORMAT_B5G5R5A1_UNORM = 86,
            FORMAT_B8G8R8A8_UNORM = 87,
            FORMAT_B8G8R8X8_UNORM = 88,
            FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
            FORMAT_B8G8R8A8_TYPELESS = 90,
            FORMAT_B8G8R8A8_UNORM_SRGB = 91,
            FORMAT_B8G8R8X8_TYPELESS = 92,
            FORMAT_B8G8R8X8_UNORM_SRGB = 93,
            FORMAT_BC6H_TYPELESS = 94,
            FORMAT_BC6H_UF16 = 95,
            FORMAT_BC6H_SF16 = 96,
            FORMAT_BC7_TYPELESS = 97,
            FORMAT_BC7_UNORM = 98,
            FORMAT_BC7_UNORM_SRGB = 99,
            FORMAT_AYUV = 100,
            FORMAT_Y410 = 101,
            FORMAT_Y416 = 102,
            FORMAT_NV12 = 103,
            FORMAT_P010 = 104,
            FORMAT_P016 = 105,
            FORMAT_420_OPAQUE = 106,
            FORMAT_YUY2 = 107,
            FORMAT_Y210 = 108,
            FORMAT_Y216 = 109,
            FORMAT_NV11 = 110,
            FORMAT_AI44 = 111,
            FORMAT_IA44 = 112,
            FORMAT_P8 = 113,
            FORMAT_A8P8 = 114,
            FORMAT_B4G4R4A4_UNORM = 115,

            FORMAT_P208 = 130,
            FORMAT_V208 = 131,
            FORMAT_V408 = 132,


            FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
            FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,


            FORMAT_FORCE_UINT = 0xffffffff
        };
    };

    struct GD_COMMON_API ResourceDimension
    {
        enum GD_COMMON_API Enum
        {
            UNKNOWN = 0,
            BUFFER = 1,
            TEXTURE1D = 2,
            TEXTURE2D = 3,
            TEXTURE3D = 4
        };
    };

    struct GD_COMMON_API HeapType
    {
        enum GD_COMMON_API Enum
        {
            DEFAULT = 1,
            UPLOAD = 2,
            READBACK = 3,
            CUSTOM = 4
        };
    };

    struct GD_COMMON_API Filter
    {
        enum GD_COMMON_API Enum
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
    };

    struct GD_COMMON_API TextureLayout
    {
        enum GD_COMMON_API Enum
        {
            LAYOUT_UNKNOWN = 0,
            LAYOUT_ROW_MAJOR = 1,
            LAYOUT_64KB_UNDEFINED_SWIZZLE = 2,
            LAYOUT_64KB_STANDARD_SWIZZLE = 3
        };
    };

    struct GD_COMMON_API ResourceFlag
    {
        enum GD_COMMON_API Enum
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

        using Flags = u32;
    };

    struct GD_COMMON_API ResourceState
    {
        enum GD_COMMON_API Enum
        {
            STATE_COMMON = 0,
            STATE_VERTEX_AND_CONSTANT_BUFFER = 0x1,
            STATE_INDEX_BUFFER = 0x2,
            STATE_RENDER_TARGET = 0x4,
            STATE_UNORDERED_ACCESS = 0x8,
            STATE_DEPTH_WRITE = 0x10,
            STATE_DEPTH_READ = 0x20,
            STATE_NON_PIXEL_SHADER_RESOURCE = 0x40,
            STATE_PIXEL_SHADER_RESOURCE = 0x80,
            STATE_STREAM_OUT = 0x100,
            STATE_INDIRECT_ARGUMENT = 0x200,
            STATE_COPY_DEST = 0x400,
            STATE_COPY_SOURCE = 0x800,
            STATE_RESOLVE_DEST = 0x1000,
            STATE_RESOLVE_SOURCE = 0x2000,
            STATE_RAYTRACING_ACCELERATION_STRUCTURE = 0x400000,
            STATE_SHADING_RATE_SOURCE = 0x1000000,
            STATE_GENERIC_READ = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
            STATE_ALL_SHADER_RESOURCE = (0x40 | 0x80),
            STATE_PRESENT = 0,
            STATE_PREDICATION = 0x200,
            STATE_VIDEO_DECODE_READ = 0x10000,
            STATE_VIDEO_DECODE_WRITE = 0x20000,
            STATE_VIDEO_PROCESS_READ = 0x40000,
            STATE_VIDEO_PROCESS_WRITE = 0x80000,
            STATE_VIDEO_ENCODE_READ = 0x200000,
            STATE_VIDEO_ENCODE_WRITE = 0x800000
        };
    };

    struct GD_COMMON_API TextureAddressMode
    {
        enum GD_COMMON_API Enum
        {
            WRAP = 1,
            MIRROR = 2,
            CLAMP = 3,
            BORDER = 4,
            MIRROR_ONCE = 5
        };
    };

    struct GD_COMMON_API InputClassification
    {
        enum GD_COMMON_API Enum
        {
            PER_VERTEX_DATA = 0,
            PER_INSTANCE_DATA = 1
        };
    };

    struct GD_COMMON_API SrvDimension
    {
        enum GD_COMMON_API Enum
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
    };

    struct GD_COMMON_API RtvDimension
    {
        enum GD_COMMON_API Enum
        {
            UNKNOWN = 0,
            BUFFER = 1,
            TEXTURE1D = 2,
            TEXTURE1DARRAY = 3,
            TEXTURE2D = 4,
            TEXTURE2DARRAY = 5,
            TEXTURE2DMS = 6,
            TEXTURE2DMSARRAY = 7,
            TEXTURE3D = 8
        };
    };

    struct GD_COMMON_API UavDimension
    {
        enum GD_COMMON_API Enum
        {
            UNKNOWN = 0,
            BUFFER = 1,
            TEXTURE1D = 2,
            TEXTURE1DARRAY = 3,
            TEXTURE2D = 4,
            TEXTURE2DARRAY = 5,
            TEXTURE3D = 8
        };
    };

    struct GD_COMMON_API DsvDimension
    {
        enum GD_COMMON_API Enum
        {
            UNKNOWN = 0,
            TEXTURE1D = 1,
            TEXTURE1DARRAY = 2,
            TEXTURE2D = 3,
            TEXTURE2DARRAY = 4,
            TEXTURE2DMS = 5,
            TEXTURE2DMSARRAY = 6
        };
    };

    struct GD_COMMON_API Blend
    {
        enum GD_COMMON_API Enum
        {
            ZERO = 1,
            ONE = 2,
            SRC_COLOR = 3,
            INV_SRC_COLOR = 4,
            SRC_ALPHA = 5,
            INV_SRC_ALPHA = 6,
            DEST_ALPHA = 7,
            INV_DEST_ALPHA = 8,
            DEST_COLOR = 9,
            INV_DEST_COLOR = 10,
            SRC_ALPHA_SAT = 11,
            BLEND_FACTOR = 14,
            INV_BLEND_FACTOR = 15,
            SRC1_COLOR = 16,
            INV_SRC1_COLOR = 17,
            SRC1_ALPHA = 18,
            INV_SRC1_ALPHA = 19
        };
    };

    struct GD_COMMON_API BlendOp
    {
        enum GD_COMMON_API Enum
        {
            ADD = 1,
            SUBTRACT = 2,
            REV_SUBTRACT = 3,
            MIN = 4,
            MAX = 5
        };
    };

    struct GD_COMMON_API LogicOp
    {
        enum GD_COMMON_API Enum
        {
            CLEAR	= 0,
            SET	= ( CLEAR + 1 ) ,
            COPY	= ( SET + 1 ) ,
            COPY_INVERTED	= ( COPY + 1 ) ,
            NOOP	= ( COPY_INVERTED + 1 ) ,
            INVERT	= ( NOOP + 1 ) ,
            AND	= ( INVERT + 1 ) ,
            NAND	= ( AND + 1 ) ,
            OR	= ( NAND + 1 ) ,
            NOR	= ( OR + 1 ) ,
            XOR	= ( NOR + 1 ) ,
            EQUIV	= ( XOR + 1 ) ,
            AND_REVERSE	= ( EQUIV + 1 ) ,
            AND_INVERTED	= ( AND_REVERSE + 1 ) ,
            OR_REVERSE	= ( AND_INVERTED + 1 ) ,
            OR_INVERTED	= ( OR_REVERSE + 1 ) 
        };
    };

    struct GD_COMMON_API ComparisonFunction
    {
        enum GD_COMMON_API Enum
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
    };

    struct GD_COMMON_API StencilOp
    {
        enum GD_COMMON_API Enum
        {
            KEEP = 1,
            ZERO = 2,
            REPLACE = 3,
            INCR_SAT = 4,
            DECR_SAT = 5,
            INVERT = 6,
            INCR = 7,
            DECR = 8
        };
    };

    struct GD_COMMON_API ColorWrite
    {
        enum GD_COMMON_API Enum
        {
            ENABLE_RED	= 1,
            ENABLE_GREEN	= 2,
            ENABLE_BLUE	= 4,
            ENABLE_ALPHA	= 8,
            ENABLE_ALL	= ( ( ( ENABLE_RED | ENABLE_GREEN )  | ENABLE_BLUE )  | ENABLE_ALPHA ) 
        };

        using Mask = u8;
    };

    struct GD_COMMON_API CullMode
    {
        enum GD_COMMON_API Enum
        {
            NONE = 1,
            FRONT = 2,
            BACK = 3
        };
    };

    struct GD_COMMON_API DsvFlag
    {
        enum GD_COMMON_API Enum
        {
            NONE = 0,
            READ_ONLY_DEPTH = 0x1,
            READ_ONLY_STENCIL = 0x2
        };
    };

    constexpr SrvDimension::Enum GetSrvDimension(ResourceDimension::Enum dim)
    {
        switch (dim)
        {
        case ResourceDimension::UNKNOWN:  return SrvDimension::UNKNOWN;
        case ResourceDimension::BUFFER:   return SrvDimension::BUFFER;
        case ResourceDimension::TEXTURE1D:return SrvDimension::TEXTURE1D;
        case ResourceDimension::TEXTURE2D:return SrvDimension::TEXTURE2D;
        case ResourceDimension::TEXTURE3D:return SrvDimension::TEXTURE3D;
        default:Assert(false); return SrvDimension::UNKNOWN;
        }
    }

    constexpr UavDimension::Enum GetUavDimension(ResourceDimension::Enum dim)
    {
        switch (dim)
        {
        case ResourceDimension::UNKNOWN:  return UavDimension::UNKNOWN;
        case ResourceDimension::BUFFER:   return UavDimension::BUFFER;
        case ResourceDimension::TEXTURE1D:return UavDimension::TEXTURE1D;
        case ResourceDimension::TEXTURE2D:return UavDimension::TEXTURE2D;
        case ResourceDimension::TEXTURE3D:return UavDimension::TEXTURE3D;
        default:Assert(false); return UavDimension::UNKNOWN;
        }
    }

    constexpr RtvDimension::Enum GetRtvDimension(ResourceDimension::Enum dim)
    {
        switch (dim)
        {
        case ResourceDimension::UNKNOWN:  return RtvDimension::UNKNOWN;
        case ResourceDimension::BUFFER:   return RtvDimension::BUFFER;
        case ResourceDimension::TEXTURE1D:return RtvDimension::TEXTURE1D;
        case ResourceDimension::TEXTURE2D:return RtvDimension::TEXTURE2D;
        case ResourceDimension::TEXTURE3D:return RtvDimension::TEXTURE3D;
        default:Assert(false); return RtvDimension::UNKNOWN;
        }
    }

    struct GD_COMMON_API Viewport
    {
        CONTINOUS_SETTER_VALUE(Viewport, f32, TopLeftX, 0.0f);
        CONTINOUS_SETTER_VALUE(Viewport, f32, TopLeftY, 0.0f);
        CONTINOUS_SETTER(Viewport, f32, Width);
        CONTINOUS_SETTER(Viewport, f32, Height);
        CONTINOUS_SETTER_VALUE(Viewport, f32, MinDepth, 0.0f);
        CONTINOUS_SETTER_VALUE(Viewport, f32, MaxDepth, 1.0f);
    };

    class IGraphicMemoryResource;

    struct GD_COMMON_API SrvDesc
    {
        using ShaderComponentMapping4 = std::array<bool, 4>;
        static ShaderComponentMapping4 FullMapping() { return { true, true, true, true }; };

        CONTINOUS_SETTER(SrvDesc, const IGraphicMemoryResource*, Resource);
        CONTINOUS_SETTER(SrvDesc, Format::Enum, Format);
        CONTINOUS_SETTER(SrvDesc, SrvDimension::Enum, ViewDimension);
        CONTINOUS_SETTER_VALUE(SrvDesc, ShaderComponentMapping4, Shader4ComponentMapping, FullMapping());
        CONTINOUS_SETTER(SrvDesc, u64, Buffer_FirstElement);
        CONTINOUS_SETTER(SrvDesc, u32, Buffer_NumElements);
        CONTINOUS_SETTER(SrvDesc, u32, Buffer_StructureByteStride);
        CONTINOUS_SETTER(SrvDesc, bool, Buffer_FlagRawRatherThanNone);
        CONTINOUS_SETTER(SrvDesc, u32, Texture2D_MostDetailedMip);
        CONTINOUS_SETTER(SrvDesc, u32, Texture2D_MipLevels);
        CONTINOUS_SETTER(SrvDesc, u32, Texture2D_PlaneSlice);
        CONTINOUS_SETTER(SrvDesc, f32, Texture2D_ResourceMinLODClamp);
    };

    struct GD_COMMON_API RtvDesc
    {
        CONTINOUS_SETTER(RtvDesc, const IGraphicMemoryResource*, Resource);
        CONTINOUS_SETTER(RtvDesc, Format::Enum, Format);
        CONTINOUS_SETTER(RtvDesc, RtvDimension::Enum, ViewDimension);
        CONTINOUS_SETTER(RtvDesc, u32, Texture2D_MipSlice);
        CONTINOUS_SETTER(RtvDesc, u32, Texture2D_PlaneSlice);
    };

    struct GD_COMMON_API DsvDesc
    {
        CONTINOUS_SETTER(DsvDesc, const IGraphicMemoryResource*, Resource);
        CONTINOUS_SETTER(DsvDesc, bool, Enabled);
        CONTINOUS_SETTER(DsvDesc, Format::Enum, Format);
        CONTINOUS_SETTER(DsvDesc, DsvDimension::Enum, ViewDimension);
        CONTINOUS_SETTER(DsvDesc, DsvFlag::Enum, Flags);
        CONTINOUS_SETTER(DsvDesc, u32, MipSlice);
    };

    struct GD_COMMON_API UavDesc
    {
        CONTINOUS_SETTER(UavDesc, const IGraphicMemoryResource*, Resource);
        CONTINOUS_SETTER(UavDesc, Format::Enum, Format);
        CONTINOUS_SETTER(UavDesc, UavDimension::Enum, ViewDimension);
        CONTINOUS_SETTER(UavDesc, u64, Buffer_FirstElement);
        CONTINOUS_SETTER(UavDesc, u32, Buffer_NumElements);
        CONTINOUS_SETTER(UavDesc, u32, Buffer_StructureByteStride);
        CONTINOUS_SETTER(UavDesc, u64, Buffer_CounterOffsetInBytes);
        CONTINOUS_SETTER(UavDesc, bool, Buffer_FlagRawRatherThanNone);
        CONTINOUS_SETTER(UavDesc, u32, Texture2D_MipSlice);
        CONTINOUS_SETTER(UavDesc, u32, Texture2D_PlaneSlice);
    };

    struct GD_COMMON_API SamplerDesc
    {
        using AddrMode3 = std::array<TextureAddressMode::Enum, 3>;
        
        CONTINOUS_SETTER(SamplerDesc, Filter::Enum, Filter);
        CONTINOUS_SETTER(SamplerDesc, AddrMode3, Address);
        CONTINOUS_SETTER_VALUE(SamplerDesc, f32, MipLODBias, 0.0f);
        CONTINOUS_SETTER_VALUE(SamplerDesc, u32, MaxAnisotropy, 0);
        CONTINOUS_SETTER_VALUE(SamplerDesc, u32, ComparisonFunc, 0);
        CONTINOUS_SETTER_VALUE(SamplerDesc, Vec4f, BorderColor, Vec4f::Zero());
        CONTINOUS_SETTER_VALUE(SamplerDesc, f32, MinLOD, 0);
        CONTINOUS_SETTER_VALUE(SamplerDesc, f32, MaxLOD, std::numeric_limits<f32>::max());
    };

    struct GD_COMMON_API InputElementDesc
    {
        CONTINOUS_SETTER(InputElementDesc, const char*, SemanticName);
        CONTINOUS_SETTER_VALUE(InputElementDesc, u32, SemanticIndex, 0);
        CONTINOUS_SETTER(InputElementDesc, Format::Enum, Format);
        CONTINOUS_SETTER_VALUE(InputElementDesc, u32, InputSlot, 0);
        CONTINOUS_SETTER(InputElementDesc, u32, AlignedByteOffset);
        CONTINOUS_SETTER_VALUE(InputElementDesc, InputClassification::Enum, InputSlotClass, InputClassification::PER_VERTEX_DATA);
        CONTINOUS_SETTER_VALUE(InputElementDesc, u32, InstanceDataStepRate, 0);
    };

    struct GD_COMMON_API VbvDesc
    {
        CONTINOUS_SETTER(VbvDesc, const IGraphicMemoryResource*, Resource);
        CONTINOUS_SETTER(VbvDesc, i32, SizeInBytes);
        CONTINOUS_SETTER(VbvDesc, i32, StrideInBytes);
    };

    struct GD_COMMON_API IbvDesc
    {
        CONTINOUS_SETTER(IbvDesc, const IGraphicMemoryResource*, Resource);
        CONTINOUS_SETTER(IbvDesc, i32, SizeInBytes);
        CONTINOUS_SETTER_VALUE(IbvDesc, Format::Enum, Format, Format::FORMAT_R16_UINT);
    };

    struct GD_COMMON_API ShaderMacro
    {
        std::string mName;
        std::string mDefinition;
    };

    struct GD_COMMON_API RasterizerDesc
    {
        // default value see CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT)
        CONTINOUS_SETTER_VALUE(RasterizerDesc, bool, FillSolidRatherThanWireframe, true);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, CullMode::Enum, CullMode, CullMode::NONE);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, bool, FrontCounterClockwise, false);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, i32, DepthBias, 0);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, f32, DepthBiasClamp, 0.0f);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, f32, SlopeScaledDepthBias, 0.0f);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, bool, DepthClipEnable, true);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, bool, MultisampleEnable, false);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, bool, AntialiasedLineEnable, false);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, i32, ForcedSampleCount, 0);
        CONTINOUS_SETTER_VALUE(RasterizerDesc, bool, ConservativeRaster, false);
    };

    struct GD_COMMON_API DepthStencilDesc
    {
        // default value see CD3DX12_DEPTH_STENCIL_DESC( CD3DX12_DEFAULT )

        struct GD_COMMON_API StencilOpDesc
        {
            CONTINOUS_SETTER_VALUE(StencilOpDesc, StencilOp::Enum, StencilFailOp, StencilOp::KEEP);
            CONTINOUS_SETTER_VALUE(StencilOpDesc, StencilOp::Enum, StencilDepthFailOp, StencilOp::KEEP);
            CONTINOUS_SETTER_VALUE(StencilOpDesc, StencilOp::Enum, StencilPassOp, StencilOp::KEEP);
            CONTINOUS_SETTER_VALUE(StencilOpDesc, ComparisonFunction::Enum, StencilFunc, ComparisonFunction::ALWAYS);
        };

        CONTINOUS_SETTER_VALUE(DepthStencilDesc, bool, DepthEnable, true);
        CONTINOUS_SETTER_VALUE(DepthStencilDesc, bool, DepthWriteAllRatherThanZero, true);
        CONTINOUS_SETTER_VALUE(DepthStencilDesc, ComparisonFunction::Enum, DepthFunc, ComparisonFunction::LESS);
        CONTINOUS_SETTER_VALUE(DepthStencilDesc, bool, StencilEnable, false);
        CONTINOUS_SETTER_VALUE(DepthStencilDesc, u8, StencilReadMask, 0xff);
        CONTINOUS_SETTER_VALUE(DepthStencilDesc, u8, StencilWriteMask, 0xff);
        StencilOpDesc FrontFace;
        StencilOpDesc BackFace;
    };

    struct GD_COMMON_API RtBlendDesc
    {
        CONTINOUS_SETTER_VALUE(RtBlendDesc, bool, BlendEnable, false);
        CONTINOUS_SETTER_VALUE(RtBlendDesc, bool, LogicOpEnable, false);
        CONTINOUS_SETTER(RtBlendDesc, Blend::Enum, SrcBlend);
        CONTINOUS_SETTER(RtBlendDesc, Blend::Enum, DestBlend);
        CONTINOUS_SETTER(RtBlendDesc, BlendOp::Enum, BlendOp);
        CONTINOUS_SETTER(RtBlendDesc, Blend::Enum, SrcBlendAlpha);
        CONTINOUS_SETTER(RtBlendDesc, Blend::Enum, DestBlendAlpha);
        CONTINOUS_SETTER(RtBlendDesc, BlendOp::Enum, BlendOpAlpha);
        CONTINOUS_SETTER(RtBlendDesc, LogicOp::Enum, LogicOp);
        CONTINOUS_SETTER_VALUE(RtBlendDesc, ColorWrite::Mask, RenderTargetWriteMask, ColorWrite::ENABLE_ALL);
    };

    struct GD_COMMON_API BlendDesc
    {
        CONTINOUS_SETTER_VALUE(BlendDesc, bool, AlphaToCoverageEnable, false);
        CONTINOUS_SETTER_VALUE(BlendDesc, bool, IndependentBlendEnable, false);
        std::array<RtBlendDesc, 8>  RtBlendDesc;
    };

    constexpr ComparisonFunction::Enum ToDepthCompareFunc(const Math::ValueCompareState& state)
    {
        if (state == Math::ValueCompareState_Equal) return ComparisonFunction::EQUAL;
        if (state == Math::ValueCompareState_Less) return ComparisonFunction::LESS;
        if (state == Math::ValueCompareState_Greater) return ComparisonFunction::GREATER;
        if (state == (Math::ValueCompareState_Equal | Math::ValueCompareState_Less)) return ComparisonFunction::LESS_EQUAL;
        if (state == (Math::ValueCompareState_Equal | Math::ValueCompareState_Greater)) return ComparisonFunction::GREATER_EQUAL;
        if (state == (Math::ValueCompareState_Less | Math::ValueCompareState_Greater)) return ComparisonFunction::NOT_EQUAL;

        Assert(false);
        return ComparisonFunction::ALWAYS;
    }

    template <typename T>
    inline std::vector<b8> ToGpuConstBufferParamData(const T& var)
    {
        std::vector<b8> result(sizeof(T), b8{});
        memcpy(result.data(), &var, sizeof(T));
        return result;
    }

    template <>
    inline std::vector<b8> ToGpuConstBufferParamData(const Mat33f& var)
    {
        std::vector<b8> result(sizeof(f32) * (4 + 4 + 3), b8{});
        Assert(false);
        return result;
    }


    template <>
    inline std::vector<b8> ToGpuConstBufferParamData(const std::vector<f32>& var)
    {
        const auto size = var.size() * sizeof(f32);

        std::vector<b8> result(size, b8{});
        memcpy_s(result.data(), size, var.data(), size);
        return result;
    }

    class GD_COMMON_API IGraphicMemoryResource
    {
    public:
        virtual HeapType::Enum          GetHeapType() const = 0;
        virtual ResourceDimension::Enum GetDimension() const = 0;
        virtual Vec3i                   GetDimSize() const = 0;
        virtual Format::Enum            GetFormat() const = 0;
        virtual u16                     GetMipLevelCount() const = 0;
    };

    class GD_COMMON_API ResouceViewUtils
    {
        static GI::RtvDesc CreateFullRtv(const IGraphicMemoryResource* resource, GI::Format::Enum rtvFormat, u32 mipSlice, u32 planeSlice);
    };

    class GD_COMMON_API MemoryResourceDesc
    {
        CONTINOUS_SETTER(MemoryResourceDesc, HeapType::Enum, HeapType);
        CONTINOUS_SETTER(MemoryResourceDesc, ResourceDimension::Enum, Dimension);
        CONTINOUS_SETTER_VALUE(MemoryResourceDesc, u64, Alignment, 0);
        CONTINOUS_SETTER(MemoryResourceDesc, u64, Width);
        CONTINOUS_SETTER(MemoryResourceDesc, u32, Height);
        CONTINOUS_SETTER(MemoryResourceDesc, u16, DepthOrArraySize);
        CONTINOUS_SETTER(MemoryResourceDesc, u16, MipLevels);
        CONTINOUS_SETTER(MemoryResourceDesc, Format::Enum, Format);
        CONTINOUS_SETTER_VALUE(MemoryResourceDesc, u64, SampleDesc_Count, 1); // see D3D12_RESOURCE_DESC.SampleDesc
        CONTINOUS_SETTER_VALUE(MemoryResourceDesc, u64, SampleDesc_Quality, 0);
        CONTINOUS_SETTER(MemoryResourceDesc, TextureLayout::Enum, Layout);
        CONTINOUS_SETTER(MemoryResourceDesc, ResourceFlag::Flags, Flags);
        CONTINOUS_SETTER_VALUE(MemoryResourceDesc, const char*, Name, nullptr);
        CONTINOUS_SETTER_VALUE(MemoryResourceDesc, ResourceState::Enum, InitState, ResourceState::STATE_COMMON);
    };

    class GD_COMMON_API IImage
    {
    public:
        virtual MemoryResourceDesc GetResourceDesc() const = 0;
    };

    class GD_COMMON_API IGraphicsInfra
    {
    public:
        virtual std::unique_ptr<IGraphicMemoryResource>     CreateMemoryResource(const MemoryResourceDesc& desc) = 0;
        virtual std::unique_ptr<IGraphicMemoryResource>     CreateMemoryResource(const IImage& image) = 0;

        virtual void                                        CopyToUploadMemoryResource(IGraphicMemoryResource* resource, const std::vector<b8>& data) = 0;

        virtual std::unique_ptr<IImage>     CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::vector<b8>& content) const = 0;
        virtual std::unique_ptr<IImage>     CreateFromScratch(Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name) const = 0;
    };

    class GD_COMMON_API GraphicsPass
    {
    public:
        template <typename T>
        void AddCbVar(const std::string& name, const T& var)
        {
            Assert(mCbParams.find(name) == mCbParams.end());
            mCbParams[name] = ToGpuConstBufferParamData(var);
        }

        void AddSrv(const std::string& name, const SrvDesc& srv)
        {
            Assert(mSrvParams.find(name) == mSrvParams.end());
            mSrvParams[name] = srv;
        }

        void AddSampler(const std::string& name, const SamplerDesc& sampler)
        {
            Assert(mSamplerParams.find(name) == mSamplerParams.end());
            mSamplerParams[name] = sampler;
        }

    public:
        struct
        {
            std::string	mFile;
            const char* mEntry = nullptr;
        }				                            mRootSignatureDesc;

        std::string                                 mVsFile;
        std::string                                 mPsFile;
        std::vector<ShaderMacro>	                mShaderMacros;

    public:
        std::array<RtvDesc, 8>               		mRtvs;
        DsvDesc                                     mDsv;

        std::vector<InputElementDesc>               mInputLayout;

        RasterizerDesc                              mRasterizerDesc;
        DepthStencilDesc                            mDepthStencilDesc;
        BlendDesc                                   mBlendDesc;

        std::vector<VbvDesc>	                    mVbvs;
        IbvDesc                 					mIbv = {};
        i32 										mVertexStartLocation = 0;
        i32 										mIndexStartLocation = 0;
        i32 										mIndexCount = 0;
        i32 										mInstanceCount = 1;

        Viewport        							mViewPort = {};
        Math::Rect  								mScissorRect = {};
        u32     									mStencilRef = 0;

    protected:
        std::map<std::string, std::vector<b8>>      mCbParams;
        std::map<std::string, SrvDesc>	            mSrvParams;
        std::map<std::string, SamplerDesc>	        mSamplerParams;
    };

    class GD_COMMON_API ComputePass
    {
    public:
        void Dispatch();

        template <typename T>
        void AddCbVar(const std::string& name, const T& var)
        {
            Assert(mCbParams.find(name) == mCbParams.end());
            mCbParams[name] = D3D12Utils::ToD3DConstBufferParamData(var);
        }

        void AddSrv(const std::string& name, const SrvDesc& srv)
        {
            Assert(mSrvParams.find(name) == mSrvParams.end());
            mSrvParams[name] = srv;
        }

        void AddUav(const std::string& name, const UavDesc& uav)
        {
            Assert(mUavParams.find(name) == mUavParams.end());
            mUavParams[name] = uav;
        }

        void AddSampler(const std::string& name, const SamplerDesc& sampler)
        {
            Assert(mSamplerParams.find(name) == mSamplerParams.end());
            mSamplerParams[name] = sampler;
        }

    public:
        struct
        {
            std::string mFile;
            const char* mEntry = nullptr;
        }					mRootSignatureDesc;

        std::string mCsFile;
        std::vector<ShaderMacro>	mShaderMacros;

    public:
        std::map<std::string, GI::SamplerDesc>		mSamplerParams;
        std::map<std::string, GI::SrvDesc>  		mSrvParams;
        std::map<std::string, GI::UavDesc>	        mUavParams;
        std::map<std::string, std::vector<byte>>	mCbParams;

        std::array<u32, 3>							mThreadGroupCounts = {};
    };

};
