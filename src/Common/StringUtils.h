#pragma once

#include "CommonTypes.h"

namespace Utils
{
	GD_COMMON_API std::wstring ToWString(const std::string& str);
	GD_COMMON_API std::wstring ToWString(const char* str);
	GD_COMMON_API std::string ToString(const std::wstring& wstr);
	GD_COMMON_API std::string ToString(const wchar_t* wstr);

	GD_COMMON_API std::string FormatString(const char* format, ...);
	GD_COMMON_API std::string EscapeString(const char* str);

	GD_COMMON_API std::string GetDirFromPath(const char* path);

	GD_COMMON_API void PrintDebugString(const char* path);

	GD_COMMON_API std::vector<b8>	LoadFileContent(const char* path);
	GD_COMMON_API void				WriteFileText(const char* path, const std::string& text);

	GD_COMMON_API u32 HashBytes(const b8* data, u32 size);

	template <typename T>
	u32 HashPod(const T& pod)
	{
		return Utils::HashBytes(reinterpret_cast<const b8*>(&pod), sizeof(T));
	}

	// https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values
	template <typename T, typename ...Args>
	inline void HashCombine(std::size_t& seed, const T& v, const Args& ... args)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		HashCombine(seed, args...);
	}

	inline void HashCombine(std::size_t& seed) {}
}

#define DEBUG_PRINT(msg, ...)	(Utils::PrintDebugString(Utils::FormatString(msg "\n", ##__VA_ARGS__ ).c_str()));
