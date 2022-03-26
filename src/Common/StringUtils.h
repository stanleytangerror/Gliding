#pragma once

#include <string>
#include <xstring>

namespace Utils
{
	GD_COMMON_API std::wstring ToWString(const std::string& str);
	GD_COMMON_API std::wstring ToWString(const char* str);
	GD_COMMON_API std::string ToString(const std::wstring& wstr);
	GD_COMMON_API std::string ToString(const wchar_t* wstr);

	GD_COMMON_API std::string FormatString(const char* format, ...);

	GD_COMMON_API std::string GetDirFromPath(const char* path);

	GD_COMMON_API void PrintDebugString(const char* path);
}

#define DEBUG_PRINT(msg, ...)	(Utils::PrintDebugString(Utils::FormatString(msg "\n", ##__VA_ARGS__ ).c_str()));
