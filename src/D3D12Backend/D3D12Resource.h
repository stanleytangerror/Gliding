#pragma once

#include "D3D12Headers.h"
#include "D3D12CommandContext.h"

namespace D3D12Backend
{
	class D3D12Device;
	class D3D12CommandContext;

	class GD_D3D12BACKEND_API IResource
	{
	public:
		virtual void Transition(D3D12Backend::D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) = 0;
		virtual ID3D12Resource* GetD3D12Resource() const = 0;
		virtual Vec3i GetSize() const = 0;
		virtual std::string GetName() const { return {}; }
	};

	class GD_D3D12BACKEND_API CommitedResource : public GI::IGraphicMemoryResource
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
			CommitedResource* Build(D3D12Device* device, GI::HeapType::Enum heapType);
		};

		class GD_D3D12BACKEND_API Possessor
		{
			CONTINOUS_SETTER(Possessor, ID3D12Resource*, Resource);
			CONTINOUS_SETTER(Possessor, const char*, Name);
			CONTINOUS_SETTER(Possessor, D3D12_RESOURCE_STATES, CurrentState);

		public:
			CommitedResource* Possess(D3D12Device* device);
		};

									~CommitedResource() override;
		void						Transition(D3D12Backend::D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState);
		ID3D12Resource*				GetD3D12Resource() const { return mResource; }
		Vec3i						GetSize() const override { return mSize; }
		GI::Format::Enum			GetFormat() const override { return D3D12Utils::ToGiFormat(mDesc.Format); }
		u16							GetMipLevelCount() const override { return mDesc.MipLevels; }
		D3D12_RESOURCE_STATES		GetState() const { return mState; }

		GI::HeapType::Enum			GetHeapType() const { return mHeapType; }
		GI::ResourceDimension::Enum	GetDimension() const { return GI::ResourceDimension::Enum(mDesc.Dimension); }

	protected:
		D3D12Device*				mDevice = nullptr;
		ID3D12Resource*				mResource = nullptr;
		Vec3i						mSize = {};
		D3D12_RESOURCE_DESC			mDesc = {};
		D3D12_RESOURCE_STATES		mState = D3D12_RESOURCE_STATE_COMMON;
		GI::HeapType::Enum			mHeapType = {};
	};
}