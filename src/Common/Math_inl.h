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
inline Mat44<T> Math::ComputeViewMatrix(const Vec3<T>& pos, const Vec3<T>& dir, const Vec3<T>& up, const Vec3<T>& right)
{
	/* {	CamRight_w	CamUp_w CamDir_w CamPos_w	} * {	Pos_v	} = {	Pos_w	}
	*			0			0		0			1				1				1
	*/
	Mat44<T> InvR = Mat44<T>::Identity();
	{
		InvR.row(0).head(3) = right;
		InvR.row(1).head(3) = up;
		InvR.row(2).head(3) = dir;
	}

	Mat44<T> InvT = Mat44<T>::Identity();
	{
		InvT.col(3).head(3) = -pos;
	}

	return InvR * InvT;
}

template <typename T, i32 Rols, i32 Cols>
inline std::string Math::ToString(const Mat<T, Rols, Cols>& mat)
{
	Eigen::IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]");

	std::stringstream ss;
	ss << mat.format(CleanFmt) << std::endl;
	return ss.str();
}

