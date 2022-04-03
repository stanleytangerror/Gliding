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
	return mWorldTransform.linear() * CamDirInViewSpace();
}
template <typename T>
Vec3<T> Math::CameraTransform<typename T>::CamUpInWorldSpace() const
{
	return mWorldTransform.linear() * CamUpInViewSpace();
}
template <typename T>
Vec3<T> Math::CameraTransform<typename T>::CamRightInWorldSpace() const
{
	return mWorldTransform.linear() * CamRightInViewSpace();
}
template <typename T>
Vec3<T> Math::CameraTransform<typename T>::CamPosInWorldSpace() const
{
	return mWorldTransform * Vec3<T>::Zero();
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
	}
}