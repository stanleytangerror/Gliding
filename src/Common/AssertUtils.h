#pragma once

#include <cassert>
#include <thread>

constexpr void Assert(const bool val)
{
#if defined(_DEBUG)
	assert(val);
#else
	(val);
#endif
}

GD_COMMON_API constexpr void AssertHResultOk(const long val);

GD_COMMON_API constexpr void AssertInThread(const std::thread::id& targetThread);

