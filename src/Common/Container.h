#pragma once

#include <map>

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