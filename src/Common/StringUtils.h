#pragma once

#include <string>
#include <xstring>

namespace Utils
{
	GD_COMMON_API std::wstring ToWString(const std::string& str);
	GD_COMMON_API std::wstring ToWString(const char* str);

	GD_COMMON_API std::string FormatString(const char* format, ...);
}

#define DebugPrint(msg, ...)	(OutputDebugString(Utils::FormatString(msg "\n", ##__VA_ARGS__ ).c_str()));
