#pragma once

#ifdef Render_Export
#define GD_RENDER_API __declspec(dllexport)
#else
#define GD_RENDER_API
#endif