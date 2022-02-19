#pragma once

#include "CommonTypes.h"

template <typename T>
struct Vec3
{
	T mX;
	T mY;
	T mZ;
};

using Vec3f = Vec3<float>;
using Vec3i = Vec3<i32>;