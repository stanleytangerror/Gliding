#include "CommonPch.h"
#include "GraphicsInfrastructure.h"

namespace GI
{

	void GraphicsPass::SetViewPortAndScissorRectToFullRt()
	{
		Assert(mRtvCount > 0 && mRtvs[0].GetResource());

		const auto size = mRtvs[0].GetResource()->GetSize();
		mViewPort.SetWidth(size.x()).SetHeight(size.y());
		mScissorRect = { 0, 0, i32(size.x()), i32(size.y()) };
	}

	void GraphicsPass::SetViewPortAndScissorRectToFullDepth()
	{
		Assert(mHasDsv && mDsv.GetResource());

		const auto size = mDsv.GetResource()->GetSize();
		mViewPort.SetWidth(size.x()).SetHeight(size.y());
		mScissorRect = { 0, 0, i32(size.x()), i32(size.y()) };
	}

	void GraphicsPass::SetGeometry(const VbvUsage& vbv, i32 vertexStartLocation, const std::vector<InputElementDesc>& inputLayout, const IbvUsage& ibv, i32 indexStartLocation, i32 indexCount, i32 instanceCount /*= 1*/)
	{
		mVbvs.push_back(vbv);
		mVertexStartLocation = vertexStartLocation;
		mInputLayout = inputLayout;

		mIbv = ibv;
		mIndexStartLocation = indexStartLocation;
		mIndexCount = indexCount;
	}

	bool GraphicsPass::IsReadyForExecute() const
	{
		for (const auto& [_, srv] : mSrvParams)
		{
			if (!srv.GetResource()->GetResourceId()) { return false; }
		}

		for (auto i = 0; i < mRtvCount; ++i)
		{
			if (!mRtvs[i].GetResource()->GetResourceId()) { return false; }
		}

		for (const auto& vbv : mVbvs)
		{
			if (!vbv.GetResource()->GetResourceId()) { return false; }
		}

		if (!mIbv.GetResource()->GetResourceId()) { return false; }

		if (mHasDsv && !mDsv.GetResource()->GetResourceId()) { return false; }

		return true;
	}

	bool ComputePass::IsReadyForExecute() const
	{
		for (const auto& [_, srv] : mSrvParams)
		{
			if (!srv.GetResource()->GetResourceId()) { return false; }
		}

		for (const auto& [_, srv] : mUavParams)
		{
			if (!srv.GetResource()->GetResourceId()) { return false; }
		}

		return true;
	}

	GI::MemoryResourceDesc::Key MemoryResourceDesc::GetKey() const
	{
		u64 hash = 0;
		Utils::HashCombine(hash, 
			mHeapType, mDimension, mAlignment,
			mWidth, mHeight, mDepthOrArraySize, mMipLevels, mFormat, 
			mSampleDesc_Count, mSampleDesc_Quality, mLayout, mFlags);
		return hash;
	}

	u32 TextureSubresourceDesc::GetSubresourceIndex(const MemoryResourceDesc& desc) const
	{
		return GetSubresourceIndex(desc.GetDimension(), desc.GetDepthOrArraySize(), desc.GetMipLevels(), 1);
	}

	u32 TextureSubresourceDesc::GetSubresourceIndex(ResourceDimension::Enum dim, u32 depthOrArrayIndex, u32 mipLevelCount, u32 planeCount) const
	{
		// https://stackoverflow.com/questions/73420449/what-is-subresource-in-direct-3d-12

		auto ArraySize = (dim != GI::ResourceDimension::TEXTURE3D)
			? depthOrArrayIndex : 1u;

		return mipLevelCount * ArraySize * PlaneIndex
			+ mipLevelCount * DepthOrArrayIndex
			+ MipLevelIndex;
	}

}