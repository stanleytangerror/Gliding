#pragma once

#include <map>

template <u64 Capacity>
struct StackMemory
{
	StackMemory() {}

	template <typename T>
	StackMemory(const T* v)
	{
		static_assert(sizeof(T) <= Capacity, "StackMemory fill overflow");
		std::memcpy(mMemory.data(), v, sizeof(T));
		mSize = sizeof(T);
	}

	StackMemory(const b8* addr, u64 size)
	{
		Assert(size <= Capacity);
		std::memcpy(mMemory.data(), addr, size);
		mSize = size;
	}
	
	template <u64 OtherCapacity>
	StackMemory(StackMemory<OtherCapacity>&& other)
	{
		static_assert(OtherCapacity <= Capacity, "StackMemory fill overflow");
		std::memcpy(mMemory.data(), other.mMemory.data(), other.mSize);
		mSize = other.mSize;
	}

	template <u64 OtherCapacity>
	StackMemory(const StackMemory<OtherCapacity>& other)
	{
		static_assert(OtherCapacity <= Capacity, "StackMemory fill overflow");
		std::memcpy(mMemory.data(), other.mMemory.data(), other.mSize);
		mSize = other.mSize;
	}

	template <u64 OtherCapacity>
	StackMemory<Capacity>& operator=(StackMemory<OtherCapacity>&& other)
	{
		static_assert(OtherCapacity <= Capacity, "StackMemory fill overflow");
		std::memcpy(mMemory.data(), other.mMemory.data(), other.mSize);
		mSize = other.mSize;
		return *this;
	}

	template <u64 OtherCapacity>
	StackMemory<Capacity>& operator=(const StackMemory<OtherCapacity>& other)
	{
		static_assert(OtherCapacity <= Capacity, "StackMemory fill overflow");
		std::memcpy(mMemory.data(), other.mMemory.data(), other.mSize);
		mSize = other.mSize;
		return *this;
	}

	const b8* GetMemory() const { return mMemory.data(); }
	u64 GetSize() const { return mSize; }

	std::array<b8, Capacity> mMemory;
	u64 mSize = 0;
};


template <typename K, typename V>
struct BijectionMap
{
public:
	bool ContainsValue(const V& v) const
	{
		return map2.find(v) != map2.end();
	}

	std::pair<K, V> GetByKey(const K& k) const
	{
		return *map1.find(k);
	}

	std::pair<K, V> GetByValue(const V& v) const
	{
		auto it = map2.find(v);
		return { it->second, it->first };
	}

	bool ContainsKey(const K& k) const
	{
		return map1.find(k) != map1.end();
	}

	void Insert(const K& k, const V& v)
	{
		map1[k] = v;
		map2[v] = k;
	}

	typename std::map<K, V>::const_iterator begin() const
	{
		return map1.begin();
	}

	typename std::map<K, V>::const_iterator end() const
	{
		return map1.end();
	}

protected:
	std::map<K, V> map1;
	std::map<V, K> map2;
};