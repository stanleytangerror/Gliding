#pragma once

#include "AssertUtils.h"
#include <sstream>

template <typename T, i32 Rols, i32 Cols>
inline bool Math::AlmostEqual(const Mat<T, Rols, Cols>& a, const Mat<T, Rols, Cols>& b)
{
	for (i32 r = 0; r < Rols; ++r)
	{
		for (i32 c = 0; c < Cols; ++c)
		{
			if (!AlmostEqual<T>(T(a(r, c)), T(b(r, c))))
			{
				return false;
			}
		}
	}

	return true;
}

template <>
inline f64 Math::Sqrt(f64 v)
{
	return sqrt(v);
}

template <>
inline f32 Math::Sqrt(f32 v)
{
	return sqrtf(v);
}

template <typename T>
Vec3<T> Math::CameraTransform<typename T>::CamDirInWorldSpace() const
{
	return mWorldRotation * CamDirInViewSpace();
}
template <typename T>
Vec3<T> Math::CameraTransform<typename T>::CamUpInWorldSpace() const
{
	return mWorldRotation * CamUpInViewSpace();
}
template <typename T>
Vec3<T> Math::CameraTransform<typename T>::CamRightInWorldSpace() const
{
	return mWorldRotation * CamRightInViewSpace();
}
template <typename T>
Vec3<T> Math::CameraTransform<typename T>::CamPosInWorldSpace() const
{
	return mWorldTranslation.vector();
}

template <typename T>
Mat44<T> Math::CameraTransform<typename T>::ComputeViewMatrix() const
{
	/*					InvViewMatrix						*		Position_view	=	Position_world
	 *	{	CamRight_w	CamUp_w	CamDir_w	CamPos_w	}	*	{		Pos_v	}	=	{	Pos_w	}
	 *	{		0			0		0			1		}		{		1		}		{	1		}
	 */

	Mat44<T> InvR = Mat44<T>::Identity();
	{
		InvR.row(0).head(3) = CamRightInWorldSpace();
		InvR.row(1).head(3) = CamUpInWorldSpace();
		InvR.row(2).head(3) = CamDirInWorldSpace();
	}

	Mat44<T> InvT = Mat44<T>::Identity();
	{
		InvT.col(3).head(3) = -CamPosInWorldSpace();
	}

	return InvR * InvT;
}

template <typename T>
Mat44<T> Math::CameraTransform<typename T>::ComputeInvViewMatrix() const
{
	/*					InvViewMatrix						*		Position_view	=	Position_world
	 *	{	CamRight_w	CamUp_w	CamDir_w	CamPos_w	}	*	{		Pos_v	}	=	{	Pos_w	}
	 *	{		0			0		0			1		}		{		1		}		{	1		}
	 */

	Mat44<T> invViewMat = Mat44<T>::Identity();
	{
		invViewMat.col(0).head(3) = CamRightInWorldSpace();
		invViewMat.col(1).head(3) = CamUpInWorldSpace();
		invViewMat.col(2).head(3) = CamDirInWorldSpace();
		invViewMat.col(3).head(3) = CamPosInWorldSpace();
	}

	return invViewMat;
}

template <typename T>
void Math::CameraTransform<typename T>::AlignCamera(const Vec3<T>& dirWorld, const Vec3<T>& upWorld, const Vec3<T>& rightWorld)
{
	Assert(AlmostZero<T>(dirWorld.dot(upWorld)));
	Assert(AlmostZero<T>(rightWorld.dot(upWorld)));
	Assert(AlmostZero<T>(rightWorld.dot(dirWorld)));
	
	/* Rotation * CamRight_w_origin = CamRight_w_new
	 * Rotation * CamDir_w_origin = CamDir_w_new
	 * Rotation * CamUp_w_origin = CamUp_w_new
	 */

	Mat33<T> originAxis;
	{
		originAxis.col(0) = Axis3DDir<T>(Axis3D_Xp);
		originAxis.col(1) = Axis3DDir<T>(Axis3D_Yp);
		originAxis.col(2) = Axis3DDir<T>(Axis3D_Zp);
	}

	Mat33<T> targetAxis;
	{
		targetAxis.col(0) = rightWorld;
		targetAxis.col(1) = dirWorld;
		targetAxis.col(2) = upWorld;
	}

	const Mat33<T>& rotMat = targetAxis * originAxis.transpose();
	mWorldRotation = rotMat;
}

template <typename T>
void Math::CameraTransform<typename T>::MoveCamera(const Vec3<T>& posWorld)
{
	mWorldTranslation = Translation<T>(posWorld);
}


template <typename T, i32 Rols, i32 Cols>
inline std::string Math::ToString(const Mat<T, Rols, Cols>& mat)
{
	Eigen::IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]");

	std::stringstream ss;
	ss << mat.format(CleanFmt) << std::endl;
	return ss.str();
}

template <typename T>
constexpr Vec3<T> Math::Axis3DDir(const Math::Axis3D& axis)
{
	switch (axis)
	{
	case Math::Axis3D_Xp: return Vec3<T>::UnitX();
	case Math::Axis3D_Xn: return Vec3<T>::UnitX() * (-1);
	case Math::Axis3D_Yp: return Vec3<T>::UnitY();
	case Math::Axis3D_Yn: return Vec3<T>::UnitY() * (-1);
	case Math::Axis3D_Zp: return Vec3<T>::UnitZ();
	case Math::Axis3D_Zn: return Vec3<T>::UnitZ() * (-1);
	default: Assert(false); return Vec3<T>::UnitZ();
	}
}

template <typename T>
Mat33<T> Math::GetRotation<T>(Axis3D source, Axis3D target, Chirality chirality)
{
	if (source == target)
	{
		return Mat33<T>::Identity();
	}

	if (std::min(source, target) % 2 == 1 && std::abs(target - source) == 1)
	{
		Math::Axis3D invAxis = std::min(source, target);
		return Scaling<T>(Math::Axis3DDir<T>(invAxis) * T(-1));
	}

	const Vec3<T>& v0 = Math::Axis3DDir<T>(source);
	const Vec3<T>& v1 = Math::Axis3DDir<T>(target);

	const Vec3<T>& rotAxis = v0.cross(v1); // v0/v1 always vertical
	Assert(rotAxis.norm());
	T ang = Math::HalfPi<T>() * (chirality == Chirality::RightHanded ? T(1) : T(-1));
	return Math::FromAngleAxis<T>(ang, rotAxis).toRotationMatrix();
}

template <typename T>
Mat44<T> Math::PerspectiveProjection<T>::ComputeProjectionMatrix() const
{
	Assert(mFovHorizontal < Math::Pi<T>()* T(2));

	const T tanHalfHorizontal = std::tan(mFovHorizontal * T(0.5));
	const T tanHalfVertical = tanHalfHorizontal / mAspectRatio;

	const T invW = T(1) / tanHalfHorizontal;
	const T invH = T(1) / tanHalfVertical;
	const T Q = mFar / (mFar - mNear);

	Mat44<T> projMat;
	{
		projMat.row(0) << invW, 0, 0, 0;
		projMat.row(1) << 0, invH, 0, 0;
		projMat.row(2) << 0, 0, Q, -Q * mNear;
		projMat.row(3) << 0, 0, 1, 0;
	}

	return projMat;
}

template <typename T>
Mat44<T> Math::PerspectiveProjection<T>::ComputeInvProjectionMatrix() const
{
	Assert(mFovHorizontal < Math::Pi<T>()* T(2));
	
	const T tanHalfHorizontal = std::tan(mFovHorizontal * T(0.5));
	const T tanHalfVertical = tanHalfHorizontal / mAspectRatio;

	const T w = tanHalfHorizontal;
	const T h = tanHalfVertical;
	const T Q = mFar / (mFar - mNear);

	Mat44<T> invProjMat;
	{
		invProjMat.row(0) << w, 0, 0, 0;
		invProjMat.row(1) << 0, h, 0, 0;
		invProjMat.row(2) << 0, 0, 0, 1;
		invProjMat.row(3) << 0, 0, T(1) / (-Q * mNear), T(1) / mNear;
	}

	return invProjMat;
}

template <typename T>
T Math::PerspectiveProjection<T>::GetFarPlaneDeviceDepth() const
{
	return T(1);
}

template <typename T>
T Math::PerspectiveProjection<T>::GetNearPlaneDeviceDepth() const
{
	return T(0);
}

template <typename T>
T Math::PerspectiveProjection<T>::GetHalfFovHorizontal() const
{
	return mFovHorizontal * T(0.5);
}

template <typename T>
T Math::PerspectiveProjection<T>::GetHalfFovVertical() const
{
	return GetHalfFovHorizontal() / mAspectRatio;
}

template <typename T>
Math::ValueCompareState Math::PerspectiveProjection<T>::GetNearerDepthCompare() const
{
	return ValueCompareState(ValueCompareState_Less | ValueCompareState_Equal);
}


template <typename T>
Mat44<T> Math::OrthographicProjection<T>::ComputeProjectionMatrix() const
{
	const float w = T(1) / (mViewWidth * T(0.5));
	const float h = T(1) / (mViewHeight * T(0.5));
	const float Q = T(1) / (mFar - mNear);

	Mat44<T> projMat;
	{
		projMat.row(0) << w, 0, 0, 0;
		projMat.row(1) << 0, h, 0, 0;
		projMat.row(2) << 0, 0, Q, -Q * mNear;
		projMat.row(3) << 0, 0, 0, 1;
	}

	return projMat;
}

template <typename T>
T Math::OrthographicProjection<T>::GetFarPlaneDeviceDepth() const
{
	return T(1);
}

template <typename T>
T Math::OrthographicProjection<T>::GetNearPlaneDeviceDepth() const
{
	return T(0);
}

template <typename T>
Math::ValueCompareState Math::OrthographicProjection<T>::GetNearerDepthCompare() const
{
	return ValueCompareState(ValueCompareState_Less | ValueCompareState_Equal);
}
