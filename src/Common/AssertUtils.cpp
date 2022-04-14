#include "CommonPch.h"
#include "AssertUtils.h"
#include <windows.h>

constexpr void AssertHResultOk(const long val)
{
	if (FAILED(val))
	{
		wchar_t buf[1024] = {};
		FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			buf,
			(sizeof(buf) / sizeof(wchar_t)),
			nullptr);

		OutputDebugString(buf);
	}

	Assert(val == S_OK);
}

GD_COMMON_API constexpr void AssertInThread(const std::thread::id& targetThread)
{
	Assert(targetThread == std::this_thread::get_id());
}
