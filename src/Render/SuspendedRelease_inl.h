#include "Common/AssertUtils.h"

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
	if (!mAvailablePool.empty())
	{
		T* result = mAvailablePool.back();
		Assert(mAliveItems.find(result) == mAliveItems.end());

		mAvailablePool.pop_back();
		mAliveItems.insert(result);
		return result;
	}
	else
	{
		T* result = mAllocFun();
		mAliveItems.insert(result);
		return result;
	}
}

template <class T>
void SuspendedReleasePool<T>::ReleaseItem(u64 releasingTime, T*& object)
{
	Assert(mAliveItems.find(object) != mAliveItems.end());
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
