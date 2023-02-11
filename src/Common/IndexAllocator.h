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

template <typename Name>
struct IndexId
{
	u64 mId = msInvalidId;

	static IndexId FromInt(u64 index) { return { index }; }
	static IndexId InvalidId() { return { msInvalidId }; }
	bool IsValid() const { return mId != msInvalidId; }
	bool operator<(const IndexId& other) const { return mId < other.mId; }

private:
	static const u64 msInvalidId = (~0x0);
};
