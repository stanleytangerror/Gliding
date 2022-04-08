#pragma once

#include "AssertUtils.h"
#include <sstream>

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