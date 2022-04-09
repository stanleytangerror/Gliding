#pragma once
#include "Pool.h"

template <class T>
Pool<T>::~Pool()
{
	Assert(mAliveItems.empty());

	for (const auto& obj : mAvailablePool)
	{
		mDeallocFun(obj);
	}
	mAvailablePool.clear();
}

template <class T>
T* Pool<T>::AllocItem()
{
	T* result = nullptr;
	if (!mAvailablePool.empty())
	{
		result = mAvailablePool.back();
		Assert(mAliveItems.find(result) == mAliveItems.end());

		mAvailablePool.pop_back();
	}
	else
	{
		result = mAllocFun();
	}

	mAliveItems.insert(result);

	return result;
}

template <class T>
void Pool<T>::ReleaseItem(T*& object)
{
	Assert(mAliveItems.find(object) != mAliveItems.end());
	mAliveItems.erase(mAliveItems.find(object));
	mResetFun(object);
	mAvailablePool.push_back(object);
	object = nullptr;
}

template <class T>
void Pool<T>::ReleaseAllActiveItems()
{
	std::unordered_set<T*> copiedItems = mAliveItems;
	for (T* item : copiedItems)
	{
		ReleaseItem(item);
	}
}