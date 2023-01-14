#pragma once

#ifdef WinGui_Export
#define WINGUI_API __declspec(dllexport)
#else
#define WINGUI_API
#endif // WinGui_Export