#include "CommonPch.h"
#include "Math.h"

Mat44f Math::PerspectiveProjection::ComputeProjectionMatrix() const
{
	const float fovVertical = mFovHorizontal / mAspectRatio;

	Assert(mFovHorizontal < Math::Pi<f32>() * 2.f);
	Assert(fovVertical < Math::Pi<f32>() * 2.f);

	const float invW = 1.f / std::tan(mFovHorizontal * 0.5f);
	const float invH = 1.f / std::tan(fovVertical * 0.5f);
	const float Q = mFar / (mFar - mNear);

	Mat44f projMat;
	{
		projMat.row(0) << invW, 0, 0, 0;
		projMat.row(1) << 0, invH, 0, 0;
		projMat.row(2) << 0, 0, Q, -Q * mNear;
		projMat.row(3) << 0, 0, 1, 0;
	}

	return projMat;
}

Mat44f Math::PerspectiveProjection::ComputeInvProjectionMatrix() const
{
	const float fovVertical = mFovHorizontal / mAspectRatio;

	Assert(mFovHorizontal < Math::Pi<f32>() * 2.f);
	Assert(fovVertical < Math::Pi<f32>() * 2.f);

	const float w = std::tan(mFovHorizontal * 0.5f);
	const float h = std::tan(fovVertical * 0.5f);
	const float Q = mFar / (mFar - mNear);

	Mat44f invProjMat;
	{
		invProjMat.row(0) << w, 0, 0, 0;
		invProjMat.row(1) << 0, h, 0, 0;
		invProjMat.row(2) << 0, 0, 0, 1;
		invProjMat.row(3) << 0, 0, 1.0 / (-Q * mNear), 1.0 / mNear;
	}

	return invProjMat;
}

f32 Math::PerspectiveProjection::GetFarPlaneDeviceDepth() const
{
	return 1.f;
}

f32 Math::PerspectiveProjection::GetNearPlaneDeviceDepth() const
{
	return 0.f;
}

f32 Math::PerspectiveProjection::GetHalfFovHorizontal() const
{
	return mFovHorizontal * 0.5f;
}

f32 Math::PerspectiveProjection::GetHalfFovVertical() const
{
	return GetHalfFovHorizontal() / mAspectRatio;
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

f32 Math::OrthographicProjection::GetFarPlaneDeviceDepth() const
{
	return 1.f;
}

f32 Math::OrthographicProjection::GetNearPlaneDeviceDepth() const
{
	return 0.f;
}

Math::ValueCompareState Math::OrthographicProjection::GetNearerDepthCompare() const
{
	return ValueCompareState(ValueCompareState_Less | ValueCompareState_Equal);
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
	return Math::FromAngleAxis<f32>(ang, rotAxis).toRotationMatrix();
}
