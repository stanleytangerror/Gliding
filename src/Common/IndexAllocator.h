#pragma once

#include <vector>
#include <atomic>
#include "CommonTypes.h"
#include "AssertUtils.h"

template <class T>
class IndexAllocator
{
public:
	IndexAllocator()
		: mWorkingThreadId(std::this_thread::get_id())
	{
	}

	T Alloc()
	{
		//Assert(mWorkingThreadId == std::this_thread::get_id());
		const u64 newIndex = mAllocedIndex++;
		return T::FromInt(newIndex);
	}

protected:
	u64 mAllocedIndex = 1;
	const std::thread::id mWorkingThreadId;
};