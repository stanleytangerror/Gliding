#pragma once

#include "D3D12Headers.h"

namespace D3D12Utils
{
	void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);
}

#define NAME_RAW_D3D12_OBJECT(x, name) ((x)->SetName(name))

