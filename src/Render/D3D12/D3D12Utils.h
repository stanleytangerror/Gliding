#pragma once

#include "D3D12Headers.h"

namespace D3D12Utils
{
	void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);
	
	ID3DBlob* LoadRs(const wchar_t* file, const char* entry);
}

#define NAME_RAW_D3D12_OBJECT(x, name) ((x)->SetName(name))

