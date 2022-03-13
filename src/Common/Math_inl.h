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

template <typename T, i32 Row>
T Vec<T, Row>::GetLength() const
{
	T result = 0;
	for (const T& e : m)
	{
		result += e * e;
	}
	return Math::Sqrt(result);
}

template <typename T, i32 Row>
Vec<T, Row> Vec<T, Row>::GetNormalized() const
{
	const T invLen = T(1) / GetLength();
	Vec<T, Row> result = {};
	for (i32 i = 0; i < Row; ++i)
	{
		result.m[i] = m[i] * invLen;
	}
	return result;
}

template <typename T, i32 Row>
Vec<T, Row> Vec<T, Row>::operator-() const
{
	Vec<T, Row> result;
	for (i32 i = 0; i < Row; ++i)
	{
		result.m[i] = -m[i];
	}
	return result;
}

template <typename T, i32 Row, i32 Col>
void Mat<T, Row, Col>::SetRow(i32 r, const Vec<T, Col>& v)
{
	Assert(0 <= r && r < Row);
	for (i32 c = 0; c < Col; ++c)
	{
		mColumns[c][r] = v[c];
	}
}

template <typename T, i32 Row, i32 Col>
void Mat<T, Row, Col>::SetCol(i32 c, const Vec<T, Row>& v)
{
	Assert(0 <= c && c < Col);
	mColumns[c] = v;
}

template <typename T, i32 Row, i32 Col>
template <i32 SubCol>
void Mat<T, Row, Col>::SetRow(i32 r, const Vec<T, SubCol>& v)
{
	static_assert(SubCol <= Col, "Cannot set content");
	Assert(0 <= r && r < Row);
	for (i32 c = 0; c < Col; ++c)
	{
		Get(r, c) = v[c];
	}
}

template <typename T, i32 Row, i32 Col>
template <i32 SubRow>
void Mat<T, Row, Col>::SetCol(i32 c, const Vec<T, SubRow>& v)
{
	static_assert(SubRow <= Row, "Cannot set content");
	Assert(0 <= c && c < Col);
	for (i32 r = 0; r < SubRow; ++r)
	{
		Get(r, c) = v[r];
	}
}

template <typename T, i32 Row, i32 Col>
Mat<T, Row, Col> Mat<T, Row, Col>::GetIdentity()
{
	static_assert(Row == Col, "Cannot get identity matrix");

	Mat<T, 4, 4> result = {};
	for (i32 i = 0; i < Row; ++i)
	{
		result.Get(i, i) = T(1);
	}
	return result;
}

template <typename T>
inline Mat44<T> Math::ComputeViewMatrix(const Vec3<T>& pos, const Vec3<T>& dir, const Vec3<T>& up, const Vec3<T>& right)
{
	Mat44<T> result = Mat44<T>::GetIdentity();
	{
		result.SetCol(0, right);
		result.SetCol(1, up);
		result.SetCol(2, dir);
		result.SetCol(3, -pos);
	}

	return result;
}

template <typename T>
inline Mat44<T> Math::ComputeModelMatrix(const Vec3<T>& pos, const Quat<T>& rot, const Vec3<T>& scale)
{
	Mat44<T> result = Mat44<T>::GetIdentity();
	result.SetCol(3, pos);
	return result;
}

