#include "CommonPch.h"
#include "StringUtils.h"
#include <cstdlib>
#include <cstdarg>

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

std::string Utils::FormatString(const char* format, ...)
{
	constexpr int BufSize = 2048;
	char buffer[BufSize + 1] = {};
	va_list ap;
	va_start(ap, format);
	vsprintf_s(buffer, BufSize, format, ap);
	return std::string(buffer);
}

