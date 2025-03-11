#pragma once

#ifdef WindowsPlatform_Export
#define WINDOWSPLATFORM_API __declspec(dllexport)
#else
#define WINDOWSPLATFORM_API
#endif // WindowsPlatform_Export

#include "Common/CommonMacros.h"
