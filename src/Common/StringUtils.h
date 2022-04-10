#pragma once

#include "CommonTypes.h"

namespace Utils
{
	GD_COMMON_API std::wstring ToWString(const std::string& str);
	GD_COMMON_API std::wstring ToWString(const char* str);
	GD_COMMON_API std::string ToString(const std::wstring& wstr);
	GD_COMMON_API std::string ToString(const wchar_t* wstr);

	GD_COMMON_API std::string FormatString(const char* format, ...);

	GD_COMMON_API std::string GetDirFromPath(const char* path);

	GD_COMMON_API void PrintDebugString(const char* path);

	GD_COMMON_API std::vector<b8>	LoadFileContent(const char* path);

	GD_COMMON_API u32 HashBytes(const b8* data, u32 size);
}

#define DEBUG_PRINT(msg, ...)	(Utils::PrintDebugString(Utils::FormatString(msg "\n", ##__VA_ARGS__ ).c_str()));
