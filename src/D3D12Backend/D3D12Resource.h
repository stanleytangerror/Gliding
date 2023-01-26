#pragma once

#include "D3D12Headers.h"
#include "D3D12CommandContext.h"

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

namespace D3D12Backend
{
	class D3D12Device;
	class D3D12CommandContext;
	class ShaderResourceView;
	class RenderTargetView;
	class UnorderedAccessView;
	class DepthStencilView;

	class GD_D3D12BACKEND_API IResource
	{
	public:
		virtual void Transition(D3D12Backend::D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) = 0;
		virtual ID3D12Resource* GetD3D12Resource() const = 0;
		virtual Vec3i GetSize() const = 0;
		virtual std::string GetName() const { return {}; }
	};

	class GD_D3D12BACKEND_API CommitedResource : public IResource
	{
	public:
		class GD_D3D12BACKEND_API Builder
		{
			CONTINOUS_SETTER(Builder, D3D12_RESOURCE_DIMENSION, Dimention);
			CONTINOUS_SETTER(Builder, u64, Alignment);
			CONTINOUS_SETTER(Builder, u64, Width);
			CONTINOUS_SETTER(Builder, u32, Height);
			CONTINOUS_SETTER(Builder, u16, DepthOrArraySize);
			CONTINOUS_SETTER(Builder, u16, MipLevels);
			CONTINOUS_SETTER(Builder, DXGI_FORMAT, Format);
			CONTINOUS_SETTER_VALUE(Builder, DXGI_SAMPLE_DESC, SampleDesc, (DXGI_SAMPLE_DESC{ 1, 0 }));
			CONTINOUS_SETTER(Builder, D3D12_TEXTURE_LAYOUT, Layout);
			CONTINOUS_SETTER(Builder, D3D12_RESOURCE_FLAGS, Flags);
			CONTINOUS_SETTER(Builder, const char*, Name);
			CONTINOUS_SETTER_VALUE(Builder, D3D12_RESOURCE_STATES, InitState, D3D12_RESOURCE_STATE_COMMON);

		public:
			CommitedResource* Build(D3D12Device* device, D3D12_HEAP_TYPE heapType);
			CommitedResource* BuildUpload(D3D12Device* device) { return Build(device, D3D12_HEAP_TYPE_UPLOAD); }
			CommitedResource* BuildDefault(D3D12Device* device) { return Build(device, D3D12_HEAP_TYPE_DEFAULT); }
			CommitedResource* BuildReadback(D3D12Device* device) { return Build(device, D3D12_HEAP_TYPE_READBACK); }
		};

		class GD_D3D12BACKEND_API Possessor
		{
			CONTINOUS_SETTER(Possessor, ID3D12Resource*, Resource);
			CONTINOUS_SETTER(Possessor, const char*, Name);
			CONTINOUS_SETTER(Possessor, D3D12_RESOURCE_STATES, CurrentState);

		public:
			CommitedResource* Possess(D3D12Device* device);
		};

		class GD_D3D12BACKEND_API SrvBuilder
		{
			CONTINOUS_SETTER(SrvBuilder, D3D12Device*, Device);
			CONTINOUS_SETTER(SrvBuilder, CommitedResource*, Resource);
			CONTINOUS_SETTER(SrvBuilder, DXGI_FORMAT, Format);
			CONTINOUS_SETTER(SrvBuilder, D3D12_SRV_DIMENSION, ViewDimension);
			CONTINOUS_SETTER_VALUE(SrvBuilder, u32, Shader4ComponentMapping, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
			CONTINOUS_SETTER(SrvBuilder, D3D12_BUFFER_SRV, Buffer);
			CONTINOUS_SETTER(SrvBuilder, D3D12_TEX1D_SRV, Texture1D);
			CONTINOUS_SETTER(SrvBuilder, D3D12_TEX1D_ARRAY_SRV, Texture1DArray);
			CONTINOUS_SETTER(SrvBuilder, D3D12_TEX2D_SRV, Texture2D);
			CONTINOUS_SETTER(SrvBuilder, D3D12_TEX2D_ARRAY_SRV, Texture2DArray);
			CONTINOUS_SETTER(SrvBuilder, D3D12_TEX2DMS_SRV, Texture2DMS);
			CONTINOUS_SETTER(SrvBuilder, D3D12_TEX2DMS_ARRAY_SRV, Texture2DMSArray);
			CONTINOUS_SETTER(SrvBuilder, D3D12_TEX3D_SRV, Texture3D);
			CONTINOUS_SETTER(SrvBuilder, D3D12_TEXCUBE_SRV, TextureCube);
			CONTINOUS_SETTER(SrvBuilder, D3D12_TEXCUBE_ARRAY_SRV, TextureCubeArray);
			CONTINOUS_SETTER(SrvBuilder, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV, RaytracingAccelerationStructure);

		public:
			D3D12Backend::ShaderResourceView* Build();
		};

		class GD_D3D12BACKEND_API RtvBuilder
		{
			CONTINOUS_SETTER(RtvBuilder, D3D12Device*, Device);
			CONTINOUS_SETTER(RtvBuilder, CommitedResource*, Resource);
			CONTINOUS_SETTER(RtvBuilder, DXGI_FORMAT, Format);
			CONTINOUS_SETTER(RtvBuilder, D3D12_RTV_DIMENSION, ViewDimension);
			CONTINOUS_SETTER(RtvBuilder, D3D12_BUFFER_RTV, Buffer);
			CONTINOUS_SETTER(RtvBuilder, D3D12_TEX1D_RTV, Texture1D);
			CONTINOUS_SETTER(RtvBuilder, D3D12_TEX1D_ARRAY_RTV, Texture1DArray);
			CONTINOUS_SETTER(RtvBuilder, D3D12_TEX2D_RTV, Texture2D);
			CONTINOUS_SETTER(RtvBuilder, D3D12_TEX2D_ARRAY_RTV, Texture2DArray);
			CONTINOUS_SETTER(RtvBuilder, D3D12_TEX2DMS_RTV, Texture2DMS);
			CONTINOUS_SETTER(RtvBuilder, D3D12_TEX2DMS_ARRAY_RTV, Texture2DMSArray);
			CONTINOUS_SETTER(RtvBuilder, D3D12_TEX3D_RTV, Texture3D);

		public:
			D3D12Backend::RenderTargetView* Build();
		};

		class GD_D3D12BACKEND_API DsvBuilder
		{
			CONTINOUS_SETTER(DsvBuilder, D3D12Device*, Device);
			CONTINOUS_SETTER(DsvBuilder, CommitedResource*, Resource);
			CONTINOUS_SETTER(DsvBuilder, DXGI_FORMAT, Format);
			CONTINOUS_SETTER(DsvBuilder, D3D12_DSV_DIMENSION, ViewDimension);
			CONTINOUS_SETTER(DsvBuilder, D3D12_DSV_FLAGS, Flags);
			CONTINOUS_SETTER(DsvBuilder, u32, MipSlice);

		public:
			D3D12Backend::DepthStencilView* BuildTex2D();
		};

		class GD_D3D12BACKEND_API UavBuilder
		{
			CONTINOUS_SETTER(UavBuilder, D3D12Device*, Device);
			CONTINOUS_SETTER(UavBuilder, CommitedResource*, Resource);
			CONTINOUS_SETTER(UavBuilder, DXGI_FORMAT, Format);
			CONTINOUS_SETTER(UavBuilder, D3D12_UAV_DIMENSION, ViewDimension);
			CONTINOUS_SETTER(UavBuilder, u64, Buffer_FirstElement);
			CONTINOUS_SETTER(UavBuilder, u32, Buffer_NumElements);
			CONTINOUS_SETTER(UavBuilder, u32, Buffer_StructureByteStride);
			CONTINOUS_SETTER(UavBuilder, u64, Buffer_CounterOffsetInBytes);
			CONTINOUS_SETTER(UavBuilder, D3D12_BUFFER_UAV_FLAGS, Buffer_Flags);
			CONTINOUS_SETTER(UavBuilder, u32, Texture2D_MipSlice);
			CONTINOUS_SETTER(UavBuilder, u32, Texture2D_PlaneSlice);

		public:
			D3D12Backend::UnorderedAccessView* BuildBuffer();
			D3D12Backend::UnorderedAccessView* BuildTex2D();
		};

		~CommitedResource();
		void						Transition(D3D12Backend::D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) override;
		ID3D12Resource* GetD3D12Resource() const override { return mResource; }
		Vec3i						GetSize() const override { return mSize; }
		DXGI_FORMAT					GetFormat() const { return mDesc.Format; } // TODO override?
		u16							GetMipLevelCount() const { return mDesc.MipLevels; } // TODO override?
		D3D12_RESOURCE_STATES		GetState() const { return mState; }
		
		SrvBuilder					CreateSrv();
		RtvBuilder					CreateRtv();
		DsvBuilder					CreateDsv();
		UavBuilder					CreateUav();

	protected:
		D3D12Device* mDevice = nullptr;
		ID3D12Resource* mResource = nullptr;
		Vec3i						mSize = {};
		D3D12_RESOURCE_DESC			mDesc = {};
		D3D12_RESOURCE_STATES		mState = D3D12_RESOURCE_STATE_COMMON;
	};

	GD_D3D12BACKEND_API CommitedResource* CreateCommitedResourceTex2D(D3D12Device* device, const Vec3i& size, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initStates, const char* name, u32 mipLevels = 1);
}