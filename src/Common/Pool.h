#pragma once


template <class T>
class Pool
{
public:
	using FAlloc = std::function<T* ()>;
	using FReset = std::function<void(T*)>;
	using FDealloc = std::function<void(T*)>;

public:
	Pool(const FAlloc& alloc, const FReset& reset, const FDealloc& dealloc)
		: mAllocFun(alloc), mResetFun(reset), mDeallocFun(dealloc) {}
	virtual ~Pool();

	T* AllocItem();
	void ReleaseItem(T*& object);
	void ReleaseAllActiveItems();

	const std::unordered_set<T*>& GetAliveItems() const { return mAliveItems; }

protected:
	FAlloc		mAllocFun;
	FReset		mResetFun;
	FDealloc	mDeallocFun;

	std::vector<T*>	mAvailablePool;
	std::unordered_set<T*> mAliveItems;
};

#include "Pool_inl.h"