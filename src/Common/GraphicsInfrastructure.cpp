#include "CommonPch.h"
#include "GraphicsInfrastructure.h"

namespace GI
{
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

}