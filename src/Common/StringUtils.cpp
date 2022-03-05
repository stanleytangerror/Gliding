#include "CommonPch.h"
#include "StringUtils.h"
#include <stdlib.h>

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
