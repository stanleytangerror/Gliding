#include "Common/Assert.h"

template <class T>
SuspendedReleasePool<T>::~SuspendedReleasePool()
{
	UpdateTime(msInfiniteTime);
	
	for (const auto& obj : mPool)
	{
		mDeallocFun(obj);
	}
	mPool.clear();
}

template <class T>
T* SuspendedReleasePool<T>::Alloc()
{
	if (!mPool.empty())
	{
		T* result = mPool.back();
		mPool.pop_back();
		return result;
	}
	else
	{
		T* result = Alloc();
		return result;
	}
}

template <class T>
void SuspendedReleasePool<T>::Release(u64 releasingTime, T*& object)
{
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
		mPool.push_back(obj);
	}
}
