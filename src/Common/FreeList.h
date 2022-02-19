#pragma once

#include <vector>
#include "CommonTypes.h"
#include "Assert.h"

template <typename T>
class FreeList
{
public:
	using Index = i32;
	enum { InvalidIndex = -1 };

	FreeList(i32 size)
		: mSize(size)
	{
		mAvailableIndices.reserve(mSize);
		for (i32 i = 0; i < mSize; ++i)
		{
			mAvailableIndices[i] = i;
		}
	}

	Index Pop()
	{
		if (mAvailableIndices.empty())
		{
			return InvalidIndex;
		}
		else
		{
			i32 result = mAvailableIndices.back();
			mAvailableIndices.pop_back();
			return result;
		}
	}

	void Push(Index idx)
	{
		Assert(mAvailableIndices.size() + 1 < mSize);
		Assert(idx != InvalidIndex && idx < mSize);
		Assert(std::find(mAvailableIndices.begin(), mAvailableIndices.end(), idx) == mAvailableIndices.end());

		mAvailableIndices.push_back(idx);
	}

	bool IsIndexValid(Index idx) const
	{
		return idx != InvalidIndex;
	}

	bool IsFull() const
	{
		return mAvailableIndices.size() == mSize;
	}

	bool IsEmpty() const
	{
		return mAvailableIndices.empty();
	}

protected:
	const i32				mSize = 0;
	std::vector<i32>		mAvailableIndices;
};