#pragma once

#include <vector>
#include "CommonTypes.h"

struct GD_COMMON_API ibytestream
{
	const b8* Data;
	const size_t Size;
	b8* Pos;

	ibytestream(const b8* data, size_t size)
		: Data(data), Size(size), Pos(const_cast<b8*>(data))
	{
	}

	template <typename T>
	ibytestream& operator>>(T& value)
	{
		read(reinterpret_cast<b8*>(&value), sizeof(T));
		return *this;
	}

	void read(b8* dest, size_t size)
	{
		std::memcpy(dest, Pos, size);
		Pos += size;
	}
};

struct GD_COMMON_API obytestream
{
	std::vector<b8> Data;
	std::vector<b8>::size_type Pos = 0;

	template <typename T>
	obytestream& operator<<(const T& value)
	{
		write(reinterpret_cast<const b8*>(&value), sizeof(T));
		return *this;
	}

	void write(const b8* src, size_t size)
	{
		Data.resize(Pos + size);
		std::memcpy(Data.data() + Pos, src, size);
		Pos += size;
	}

	std::vector<b8> to_bytes() const
	{
		return Data;
	}
};
