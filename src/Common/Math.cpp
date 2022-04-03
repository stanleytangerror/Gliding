#include "CommonPch.h"
#include "Math.h"

Mat44f Math::PerspectiveProjection::ComputeProjectionMatrix() const
{
	const float fovVertical = mFovHorizontal / mAspectRatio;
	const float w = 1.f / std::tan(mFovHorizontal * 0.5f);
	const float h = 1.f / std::tan(fovVertical * 0.5f);
	const float Q = mFar / (mFar - mNear);

	Mat44f projMat;
	{
		projMat.row(0) << w, 0, 0, 0;
		projMat.row(1) << 0, h, 0, 0;
		projMat.row(2) << 0, 0, Q, -Q * mNear;
		projMat.row(3) << 0, 0, 1, 0;
	}

	return projMat;
}

f32 Math::PerspectiveProjection::GetFarPlaneDepth() const
{
	return 1.f;
}

f32 Math::PerspectiveProjection::GetNearPlaneDepth() const
{
	return 0.f;
}

f32 Math::PerspectiveProjection::GetFovHorizontal() const
{
	return mFovHorizontal;
}

f32 Math::PerspectiveProjection::GetFovVertical() const
{
	return mFovHorizontal / mAspectRatio;
}

Math::ValueCompareState Math::PerspectiveProjection::GetNearerDepthCompare() const
{
	return ValueCompareState(ValueCompareState_Less | ValueCompareState_Equal);
}

Mat44f Math::OrthographicProjection::ComputeProjectionMatrix() const
{
	const float w = 1.f / (mViewWidth * 0.5f);
	const float h = 1.f / (mViewHeight * 0.5f);
	const float Q = 1.f / (mFar - mNear);

	Mat44f projMat;
	{
		projMat.row(0) << w, 0, 0, 0;
		projMat.row(1) << 0, h, 0, 0;
		projMat.row(2) << 0, 0, Q, -Q * mNear;
		projMat.row(3) << 0, 0, 0, 1;
	}

	return projMat;
}

Mat33f Math::GetRotation(Axis3D source, Axis3D target, Chirality chirality)
{
	if (source == target)
	{
		return Mat33f::Identity();
	}

	if (std::min(source, target) % 2 == 1 && std::abs(target - source) == 1)
	{
		Math::Axis3D invAxis = std::min(source, target);
		return Scalingf(Math::Axis3DDir<f32>(invAxis) * (-1));
	}

	const Vec3f& v0 = Math::Axis3DDir<f32>(source);
	const Vec3f& v1 = Math::Axis3DDir<f32>(target);

	const Vec3f& rotAxis = v0.cross(v1); // v0/v1 always vertical
	Assert(rotAxis.norm());
	f32 ang = Math::HalfPi<f32>() * (chirality == Chirality::RightHanded ? 1.f : (-1.f));
	return Rotationf{ ang, rotAxis }.toRotationMatrix();
}
