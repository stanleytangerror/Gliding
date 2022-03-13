#pragma once

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
