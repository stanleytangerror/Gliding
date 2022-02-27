#pragma once

#include "CommonTypes.h"
#include <array>

template <typename T, i32 Row>
struct Vec
{
	std::array<T, Row> m;
};

using Vec3f = Vec<float, 3>;
using Vec3i = Vec<i32, 3>;

template <typename T, i32 Row, i32 Col>
struct Mat
{
	std::array<Vec<T, Row>, Col> m;
};

using Mat33f = Mat<float, 3, 3>;
using Mat44f = Mat<float, 4, 4>;

template <typename T>
constexpr T Align(T Val, u64 Alignment)
{
	return (T)(((u64)Val + Alignment - 1) & ~(Alignment - 1));
}
