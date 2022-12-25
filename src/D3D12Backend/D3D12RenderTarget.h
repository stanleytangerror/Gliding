#pragma once

#include "D3D12Resource.h"
#include "D3D12ResourceView.h"

class GD_D3D12BACKEND_API D3D12RenderTarget : public ID3D12Res
{
public:
	D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, const char* name);
	D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, i32 mipLevelCount, const char* name);
	D3D12RenderTarget(D3D12Device* device, i32 count, i32 stride, DXGI_FORMAT format, const char* name);
	virtual ~D3D12RenderTarget();

	void		Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) override;

	RTV* CreateTex2DRtv();
	RTV* CreateTex2DArrayRtv(uint32_t firstArrayIdx, uint32_t arrayCount);
	SRV* CreateTexCubeSrv();
	//DSV* CreateTex2DDsv();

	RTV* GetRtv() const { return mRtv; }
	SRV* GetSrv() const { return mSrv; }
	UAV* GetUav() const { return mUav; }

	DXGI_FORMAT							GetFormat() const { return mFormat; }
	D3D12_RESOURCE_STATES				GetResStates() const { return mState; }
	ID3D12Resource*						GetD3D12Resource() const override { return mResource; }
	Vec3i								GetSize() const override { return mSize; }
	i32									GetMipLevelCount() const { return mMipLevelCount; }

	void								Clear(D3D12CommandContext* context, const Vec4f& color);

protected:
	D3D12Device*				mDevice = nullptr;
	ID3D12Resource*				mResource = nullptr;
	Vec3i						mSize = {};
	i32							mMipLevelCount = 1;
	DXGI_FORMAT					mFormat;
	D3D12_RESOURCE_STATES		mState = D3D12_RESOURCE_STATE_COMMON;

	RTV*						mRtv = nullptr;
	UAV*						mUav = nullptr;
	SRV*						mSrv = nullptr;
};

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
	class GD_D3D12BACKEND_API CommitedResource : public ID3D12Res
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
			CommitedResource* Build(D3D12Device* device);
		};

		class GD_D3D12BACKEND_API SrvBuilder
		{
			CONTINOUS_SETTER(SrvBuilder, D3D12Device*, Device);
			CONTINOUS_SETTER(SrvBuilder, CommitedResource*, Resource);
			CONTINOUS_SETTER(SrvBuilder, DXGI_FORMAT, Format);
			CONTINOUS_SETTER(SrvBuilder, D3D12_SRV_DIMENSION, ViewDimension);
			CONTINOUS_SETTER_VALUE(SrvBuilder, u32, Shader4ComponentMapping, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
			CONTINOUS_SETTER(SrvBuilder, u32, MostDetailedMip);
			CONTINOUS_SETTER(SrvBuilder, u32, MipLevels);
			CONTINOUS_SETTER(SrvBuilder, u32, PlaneSlice);
			CONTINOUS_SETTER(SrvBuilder, f32, ResourceMinLODClamp);

		public:
			SRV* BuildTex2D();
		};

		class GD_D3D12BACKEND_API RtvBuilder
		{
			CONTINOUS_SETTER(RtvBuilder, D3D12Device*, Device);
			CONTINOUS_SETTER(RtvBuilder, CommitedResource*, Resource);
			CONTINOUS_SETTER(RtvBuilder, DXGI_FORMAT, Format);
			CONTINOUS_SETTER(RtvBuilder, D3D12_RTV_DIMENSION, ViewDimension);
			CONTINOUS_SETTER(RtvBuilder, u32, MipSlice);
			CONTINOUS_SETTER(RtvBuilder, u32, PlaneSlice);

		public:
			RTV* BuildTex2D();
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
			DSV* BuildTex2D();
		};

		~CommitedResource();
		void						Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) override;
		ID3D12Resource*				GetD3D12Resource() const override { return mResource; }
		Vec3i						GetSize() const override { return mSize; }
		DXGI_FORMAT					GetFormat() const { return mFormat; } // TODO override?

		SrvBuilder					CreateSrv();
		RtvBuilder					CreateRtv();
		DsvBuilder					CreateDsv();

	protected:
		D3D12Device*				mDevice = nullptr;
		ID3D12Resource*				mResource = nullptr;
		Vec3i						mSize = {};
		i32							mMipLevelCount = 1;
		DXGI_FORMAT					mFormat = DXGI_FORMAT_UNKNOWN;
		D3D12_RESOURCE_STATES		mState = D3D12_RESOURCE_STATE_COMMON;
	};

	GD_D3D12BACKEND_API CommitedResource*				CreateCommitedResourceTex2D(D3D12Device* device, const Vec3i& size, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initStates, const char* name);
}