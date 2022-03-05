#pragma once

#ifdef Common_Export
#define GD_COMMON_API __declspec(dllexport)
#else
#define GD_COMMON_API
#endif