#pragma once

#include "CommonTypes.h"
#include <unordered_set>
#include <queue>

template <class T>
class SuspendedReleasePool
{
public:
	using FAlloc = std::function<T*()>;
	using FReset = std::function<void(T*)>;
	using FDealloc = std::function<void(T*)>;

public:
	SuspendedReleasePool(const FAlloc& alloc, const FReset& reset, const FDealloc& dealloc) 
		: mAllocFun(alloc), mResetFun(reset), mDeallocFun(dealloc) {}
	virtual ~SuspendedReleasePool();

	T* AllocItem();
	void ReleaseItem(u64 releasingTime, T*& object);
	void ReleaseAllActiveItems(u64 releasingTime);

	void UpdateTime(u64 time);

	const std::unordered_set<T*>& GetAliveItems() const { return mAliveItems; }

protected:
	static const u64 msInfiniteTime = u64(-1);

	struct SuspendedInfo
	{
		uint64_t	mReleasingTime = msInfiniteTime;
		T*			mObj = nullptr;

		SuspendedInfo(uint64_t releasingTime, T* obj)
			: mReleasingTime(releasingTime)
			, mObj(obj)
		{}

		bool operator > (const SuspendedInfo& other) const
		{
			return mReleasingTime > other.mReleasingTime;
		}
	};

	FAlloc		mAllocFun;
	FReset		mResetFun;
	FDealloc	mDeallocFun;

	std::vector<T*>	mAvailablePool;
	std::priority_queue<SuspendedInfo, std::vector< SuspendedInfo>, std::greater<SuspendedInfo>> mSuspendQueue;
	std::unordered_set<T*> mAliveItems;
	u64	mCurrentTime = 0;
};

#include "SuspendedRelease_inl.h"
