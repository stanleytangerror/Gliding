#pragma once

#include <string>
#include <xstring>

namespace Utils
{
	GD_COMMON_API std::wstring ToWString(const std::string& str);
	GD_COMMON_API std::wstring ToWString(const char* str);
}