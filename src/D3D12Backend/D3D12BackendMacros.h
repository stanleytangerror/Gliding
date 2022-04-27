#pragma once

#ifdef D3D12Backend_Export
#define GD_D3D12BACKEND_API __declspec(dllexport)
#else
#define GD_D3D12BACKEND_API
#endif