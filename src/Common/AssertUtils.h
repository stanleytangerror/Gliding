#pragma once

#include <cassert>

constexpr void Assert(const bool val)
{
#if defined(_DEBUG)
	assert(val);
#else
	(val);
#endif
}

GD_COMMON_API constexpr void AssertHResultOk(const long val);
