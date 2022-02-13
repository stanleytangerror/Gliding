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

constexpr void AssertHResultOk(const long val)
{
	Assert(val == 0);
}