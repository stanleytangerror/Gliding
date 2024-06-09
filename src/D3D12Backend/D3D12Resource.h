#pragma once

#include "D3D12Headers.h"
#include "D3D12CommandContext.h"
#include "D3D12ResourceManager.h"

namespace D3D12Backend
{
	class D3D12Device;
	class D3D12CommandContext;

	class GraphicMemoryResource : public GI::IGraphicMemoryResource
	{
	public:
									GraphicMemoryResource(D3D12Device* device, GI::CommittedResourceId id);
									~GraphicMemoryResource() override;

		GI::HeapType::Enum			GetHeapType() const override;
		GI::ResourceDimension::Enum	GetDimension() const override;
		Vec3u						GetSize() const override;
		GI::Format::Enum			GetFormat() const override;
		u16							GetMipLevelCount() const override;
	
	protected:
		D3D12Device*				mDevice = nullptr;
	};

	class CommitedResource
	{
		friend ResourceManager;
	public:
									~CommitedResource() ;
		void						Transition(D3D12Backend::D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState);
		ID3D12Resource*				GetD3D12Resource() const { return mResource; }
		Vec3u						GetSize() const { return mSize; }
		GI::Format::Enum			GetFormat() const { return D3D12Utils::ToGiFormat(mDesc.Format); }
		u16							GetMipLevelCount() const { return mDesc.MipLevels; }
		D3D12_RESOURCE_STATES		GetState() const { return mState; }

		GI::HeapType::Enum			GetHeapType() const { return mHeapType; }
		GI::ResourceDimension::Enum	GetDimension() const { return GI::ResourceDimension::Enum(mDesc.Dimension); }

	protected:
		D3D12Device*				mDevice = nullptr;
		ID3D12Resource*				mResource = nullptr;

		Vec3u						mSize = {};
		D3D12_RESOURCE_DESC			mDesc = {};
		D3D12_RESOURCE_STATES		mState = D3D12_RESOURCE_STATE_COMMON;
		GI::HeapType::Enum			mHeapType = {};
	};
}