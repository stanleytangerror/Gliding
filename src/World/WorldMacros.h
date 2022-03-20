#pragma once

#ifdef World_Export
#define GD_WORLD_API __declspec(dllexport)
#else
#define GD_WORLD_API
#endif
