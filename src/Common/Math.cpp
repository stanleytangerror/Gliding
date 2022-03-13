#include "CommonPch.h"
#include "Math.h"

Mat44f Math::PerspectiveProjection::ComputeProjectionMatrix()
{
	const float fovw = mFovh * mAspectRatio;
	const float w = 1.f / std::tan(fovw * 0.5f);
	const float h = 1.f / std::tan(mFovh * 0.5f);
	const float Q = mFar / (mFar - mNear);

	Mat44f projMat;
	{
		projMat.SetRow(0, { w, 0, 0, 0 });
		projMat.SetRow(1, { 0, h, 0, 0 });
		projMat.SetRow(2, { 0, 0, Q, -Q * mNear });
		projMat.SetRow(3, { 0, 0, 1, 0 });
	}

	return projMat;
}

Mat44f Math::OrthographicProjection::ComputeProjectionMatrix()
{
	const float w = 1.f / (mViewWidth * 0.5f);
	const float h = 1.f / (mViewHeight * 0.5f);
	const float Q = 1.f / (mFar - mNear);

	Mat44f projMat;
	{
		projMat.SetRow(0, { w, 0, 0, 0 });
		projMat.SetRow(1, { 0, h, 0, 0 });
		projMat.SetRow(2, { 0, 0, Q, -Q * mNear });
		projMat.SetRow(3, { 0, 0, 0, 1 });
	}

	return projMat;
}