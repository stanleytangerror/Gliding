#pragma once

#include "AssertUtils.h"

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
	Mat44<T> result = Mat44<T>::Identity();
	{
		result.col(0).head(3) = right;
		result.col(1).head(3) = up;
		result.col(2).head(3) = dir;
		result.col(3).head(3) = -pos;
	}

	return result;
}

template <typename T>
inline Mat44<T> Math::ComputeModelMatrix(const Vec3<T>& pos, const Quat<T>& rot, const Vec3<T>& scale)
{
	Mat44<T> result = Mat44<T>::Identity();
	result.col(3).head(3) = pos;
	return result;
}

