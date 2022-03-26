#include "CommonPch.h"
#include "StringUtils.h"
#include <cstdlib>
#include <cstdarg>
#include <windows.h>

std::wstring Utils::ToWString(const std::string& str)
{
	std::wstring ws(str.size(), L' '); // Overestimate number of code points.
	ws.resize(std::mbstowcs(&ws[0], str.c_str(), str.size())); // Shrink to fit
	return ws;
}

std::wstring Utils::ToWString(const char* str)
{
	return ToWString(std::string(str));
}

GD_COMMON_API std::string Utils::ToString(const std::wstring& wstr)
{
	std::string str(wstr.size(), L' '); // Overestimate number of code points.
	str.resize(std::wcstombs(&str[0], wstr.c_str(), wstr.size())); // Shrink to fit
	return str;
}

GD_COMMON_API std::string Utils::ToString(const wchar_t* wstr)
{
	return ToString(std::wstring(wstr));
}

std::string Utils::FormatString(const char* format, ...)
{
	constexpr int BufSize = 2048;
	char buffer[BufSize + 1] = {};
	va_list ap;
	va_start(ap, format);
	vsprintf_s(buffer, BufSize, format, ap);
	return std::string(buffer);
}

GD_COMMON_API std::string Utils::GetDirFromPath(const char* path)
{
	const std::string& pathStr = path;
	size_t pos = pathStr.find_last_of("\\/");
	return (std::string::npos == pos)
		? ""
		: pathStr.substr(0, pos);
}

GD_COMMON_API void Utils::PrintDebugString(const char* path)
{
	OutputDebugString(Utils::ToWString(path).c_str());
}

