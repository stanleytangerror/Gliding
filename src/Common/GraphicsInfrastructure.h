#pragma once

#define CAT2(X,Y) X##Y
#define CAT(X,Y) CAT2(X,Y)

#define SETTER(Type, Name) \
	protected:	Type m##Name = {}; \
	public:		using CAT(Temp, __LINE__) = Type; \
				void Set##Name(const CAT(Temp, __LINE__) & Name) { m##Name = Name; }
#define CONTINOUS_SETTER(Class, Type, Name) \
	protected:	Type m##Name = {}; \
	public:		using CAT(Temp, __LINE__) = Type; \
				Class& Set##Name(const CAT(Temp, __LINE__) & Name) { m##Name = Name; return *this; }
#define CONTINOUS_SETTER_VALUE(Class, Type, Name, DefValue)	\
	protected:	Type m##Name = (DefValue); \
	public:		using CAT(Temp, __LINE__) = Type; \
				Class& Set##Name(const CAT(Temp, __LINE__) & Name) { m##Name = Name; return *this;  }

namespace GI 
{
    struct Format
    {
        enum Enum
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

    struct Filter
    {
        enum Enum
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

    struct TextureAddressMode
    {
        enum Enum
        {
            WRAP = 1,
            MIRROR = 2,
            CLAMP = 3,
            BORDER = 4,
            MIRROR_ONCE = 5
        };
    };

    struct InputClassification
    {
        enum Enum
        {
            PER_VERTEX_DATA = 0,
            PER_INSTANCE_DATA = 1
        };
    };

    struct SrvDimension
    {
        enum Enum
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

    struct RtvDimension
    {
        enum Enum
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

    struct UavDimension
    {
        enum Enum
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

    struct DsvDimension
    {
        enum Enum
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

    struct ComparisonFunction
    {
        enum Enum
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

    struct StencilOp
    {
        enum Enum
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

    struct CullMode
    {
        enum Enum
        {
            NONE = 1,
            FRONT = 2,
            BACK = 3
        };
    };

    struct DsvFlag
    {
        enum Enum
        {
            NONE = 0,
            READ_ONLY_DEPTH = 0x1,
            READ_ONLY_STENCIL = 0x2
        };
    };

    struct Viewport
    {
        f32 TopLeftX;
        f32 TopLeftY;
        f32 Width;
        f32 Height;
        f32 MinDepth;
        f32 MaxDepth;
    };

    struct GD_COMMON_API SrvDesc
    {
        CONTINOUS_SETTER(SrvDesc, Format::Enum, Format);
        CONTINOUS_SETTER(SrvDesc, SrvDimension::Enum, ViewDimension);
        CONTINOUS_SETTER_VALUE(SrvDesc, u32, Shader4ComponentMapping, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
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
        CONTINOUS_SETTER(RtvDesc, Format::Enum, Format);
        CONTINOUS_SETTER(RtvDesc, RtvDimension::Enum, ViewDimension);
        CONTINOUS_SETTER(RtvDesc, u32, MipSlice);
        CONTINOUS_SETTER(RtvDesc, u32, PlaneSlice);
    };

    struct GD_D3D12BACKEND_API DsvDesc
    {
        CONTINOUS_SETTER(DsvDesc, bool, Enabled);
        CONTINOUS_SETTER(DsvDesc, Format::Enum, Format);
        CONTINOUS_SETTER(DsvDesc, DsvDimension::Enum, ViewDimension);
        CONTINOUS_SETTER(DsvDesc, DsvFlag::Enum, Flags);
        CONTINOUS_SETTER(DsvDesc, u32, MipSlice);
    };

    struct GD_D3D12BACKEND_API UavDesc
    {
        CONTINOUS_SETTER(UavDesc, UavDimension, ViewDimension);
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
        CONTINOUS_SETTER_VALUE(SamplerDesc, f32, MipLODBias, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
        CONTINOUS_SETTER(SamplerDesc, u32, MaxAnisotropy);
        CONTINOUS_SETTER(SamplerDesc, u32, ComparisonFunc);
        CONTINOUS_SETTER(SamplerDesc, Vec4f, BorderColor);
        CONTINOUS_SETTER_VALUE(SamplerDesc, f32, MinLOD, 0);
        CONTINOUS_SETTER_VALUE(SamplerDesc, f32, MaxLOD, std::numeric_limits<f32>::max());
    };

    struct GD_COMMON_API InputElementDesc
    {
        CONTINOUS_SETTER(InputElementDesc, const char*, SemanticName);
        CONTINOUS_SETTER(InputElementDesc, u32, SemanticIndex);
        CONTINOUS_SETTER(InputElementDesc, Format::Enum, Format);
        CONTINOUS_SETTER(InputElementDesc, u32, InputSlot);
        CONTINOUS_SETTER(InputElementDesc, u32, AlignedByteOffset);
        CONTINOUS_SETTER(InputElementDesc, InputClassification::Enum, InputSlotClass);
        CONTINOUS_SETTER(InputElementDesc, u32, InstanceDataStepRate);
    };

    struct GD_COMMON_API VbvDesc
    {
        CONTINOUS_SETTER(VbvDesc, i32, SizeInBytes);
        CONTINOUS_SETTER(VbvDesc, i32, StrideInBytes);
    };

    struct GD_COMMON_API IbvDesc
    {
        CONTINOUS_SETTER(IbvDesc, i32, SizeInBytes);
        CONTINOUS_SETTER_VALUE(IbvDesc, Format::Enum, StrideInBytes, Format::FORMAT_R16_UINT);
    };

    struct GD_COMMON_API ShaderMacro
    {
        std::string mName;
        std::string mDefinition;
    };

    struct GD_COMMON_API RasterizerDesc
    {
        CONTINOUS_SETTER(RasterizerDesc, bool, FillSolidRatherThanWireframe);
        CONTINOUS_SETTER(RasterizerDesc, CullMode, CullMode);
        CONTINOUS_SETTER(RasterizerDesc, bool, FrontCounterClockwise);
        CONTINOUS_SETTER(RasterizerDesc, i32, DepthBias);
        CONTINOUS_SETTER(RasterizerDesc, f32, DepthBiasClamp);
        CONTINOUS_SETTER(RasterizerDesc, f32, SlopeScaledDepthBias);
        CONTINOUS_SETTER(RasterizerDesc, bool, DepthClipEnable);
        CONTINOUS_SETTER(RasterizerDesc, bool, MultisampleEnable);
        CONTINOUS_SETTER(RasterizerDesc, bool, AntialiasedLineEnable);
        CONTINOUS_SETTER(RasterizerDesc, i32, ForcedSampleCount);
        CONTINOUS_SETTER(RasterizerDesc, bool, ConservativeRaster);
    };

    struct GD_COMMON_API DepthStencilDesc
    {
        CONTINOUS_SETTER(DepthStencilDesc, bool, DepthEnable);
        CONTINOUS_SETTER(DepthStencilDesc, bool, DepthWriteAllRatherThanZero);
        CONTINOUS_SETTER(DepthStencilDesc, ComparisonFunction::Enum, DepthFunc);
        CONTINOUS_SETTER(DepthStencilDesc, bool, StencilEnable);
        CONTINOUS_SETTER(DepthStencilDesc, u8, StencilReadMask);
        CONTINOUS_SETTER(DepthStencilDesc, u8, StencilWriteMask);

        CONTINOUS_SETTER(DepthStencilDesc, StencilOp::Enum, FrontFace_StencilFailOp);
        CONTINOUS_SETTER(DepthStencilDesc, StencilOp::Enum, FrontFace_StencilDepthFailOp);
        CONTINOUS_SETTER(DepthStencilDesc, StencilOp::Enum, FrontFace_StencilPassOp);
        CONTINOUS_SETTER(DepthStencilDesc, ComparisonFunction::Enum, FrontFace_StencilFunc);

        CONTINOUS_SETTER(DepthStencilDesc, StencilOp::Enum, BackFace_StencilFailOp);
        CONTINOUS_SETTER(DepthStencilDesc, StencilOp::Enum, BackFace_StencilDepthFailOp);
        CONTINOUS_SETTER(DepthStencilDesc, StencilOp::Enum, BackFace_StencilPassOp);
        CONTINOUS_SETTER(DepthStencilDesc, ComparisonFunction::Enum, BackFace_StencilFunc);
    };

    template <typename T>
    inline std::vector<byte> ToGpuConstBufferParamData(const T& var)
    {
        std::vector<byte> result(sizeof(T), 0);
        memcpy(result.data(), &var, sizeof(T));
        return result;
    }

    template <>
    inline std::vector<byte> ToGpuConstBufferParamData(const Mat33f& var)
    {
        std::vector<byte> result(sizeof(f32) * (4 + 4 + 3), 0);
        Assert(false);
        return result;
    }


    template <>
    inline std::vector<byte> ToGpuConstBufferParamData(const std::vector<f32>& var)
    {
        const auto size = var.size() * sizeof(f32);

        std::vector<byte> result(size, 0);
        memcpy_s(result.data(), size, var.data(), size);
        return result;
    }


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
        std::map<u8, RtvDesc>               		mRtvs;
        DsvDesc                                     mDsv;

        std::vector<InputElementDesc>               mVbElements;

        RasterizerDesc                              mRasterizerDesc;
        DepthStencilDesc                            mDepthStencilDesc;

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
        std::map<std::string, std::vector<byte>>	mCbParams;
        std::map<std::string, SrvDesc>	            mSrvParams;
        std::map<std::string, SamplerDesc>	        mSamplerParams;
    };
};
