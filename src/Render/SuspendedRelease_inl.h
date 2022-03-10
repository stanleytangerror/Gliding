#pragma once
#include "SuspendedRelease.h"

template <class T>
SuspendedReleasePool<T>::~SuspendedReleasePool()
{
	Assert(mSuspendQueue.empty());
	Assert(mAliveItems.empty());
	
	for (const auto& obj : mAvailablePool)
	{
		mDeallocFun(obj);
	}
	mAvailablePool.clear();
}

template <class T>
T* SuspendedReleasePool<T>::AllocItem()
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
void SuspendedReleasePool<T>::ReleaseItem(u64 releasingTime, T*& object)
{
	Assert(mAliveItems.find(object) != mAliveItems.end());
	mAliveItems.erase(mAliveItems.find(object));
	mSuspendQueue.emplace(releasingTime, object);
	object = nullptr;
}

template <class T>
void SuspendedReleasePool<T>::UpdateTime(u64 time)
{
	Assert(mCurrentTime <= time);
	mCurrentTime = time;
	
	while (!mSuspendQueue.empty() && mSuspendQueue.top().mReleasingTime <= time)
	{
		T* obj = mSuspendQueue.top().mObj;
		mSuspendQueue.pop();

		mResetFun(obj);
		mAvailablePool.push_back(obj);
	}
}

template <class T>
void SuspendedReleasePool<T>::ReleaseAllActiveItems(u64 releasingTime)
{
	std::unordered_set<T*> copiedItems = mAliveItems;
	for (T* item : copiedItems)
	{
		ReleaseItem(releasingTime, item);
	}
}