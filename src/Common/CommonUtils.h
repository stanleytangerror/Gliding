#pragma once

namespace Utils
{
	template <typename T>
	void SafeDelete(T*& ptr)
	{
		if (ptr)
		{
			delete ptr;
			ptr = nullptr;
		}
	}

	template <typename T>
	void SafeRelease(T*& ptr)
	{
		if (ptr)
		{
			ptr->Release();
			ptr = nullptr;
		}
	}

	template <typename T, std::size_t Size>
	std::size_t GetArrayLength(T(&)[Size]) { return Size; }
}