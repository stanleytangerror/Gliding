#include "CommonPch.h"
#include "StringUtils.h"
#include "AssertUtils.h"
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

GD_COMMON_API std::vector<b8> Utils::LoadFileContent(const char* path)
{
	std::ifstream is(path, std::ifstream::binary);
	if (is)
	{
		// get length of file:
		is.seekg(0, is.end);
		i64 length = is.tellg();
		is.seekg(0, is.beg);

		std::vector<b8> result(length);
		// read data as a block:
		is.read(reinterpret_cast<char*>(result.data()), length);

		Assert(is.operator bool());
		is.close();

		return result;
	}

	return {};
}

GD_COMMON_API u32 Utils::HashBytes(const b8* data, u32 size)
{
	/* Fowler¨CNoll¨CVo hash function */
	const u32 fnv_prime = 16777619u;
	const u32 offset_basis = 2166136261u;
	u32 hash = offset_basis;

	for (u32 i = 0; i < size; i++)
	{
		hash ^= u32(data[i]);
		hash *= fnv_prime;
	}

	return hash;
}

