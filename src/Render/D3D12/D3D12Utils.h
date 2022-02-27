#pragma once

#include "D3D12Headers.h"

namespace D3D12Utils
{
	void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);
	
	ID3DBlob* LoadRs(const char* filePath, const char* entry);
	ID3DBlob* LoadVs(const char* filePath);
	ID3DBlob* LoadPs(const char* filePath);
	ID3DBlob* LoadCs(const char* filePath);

	ID3DBlob* CompileBlobFromFile(const char* filePath, const char* entryName, const char* target, u32 flags);
	ID3DBlob* CompileBlob(const void* date, const i32 dataSize, const char* entryName, const char* target, u32 flags);

}

#define NAME_RAW_D3D12_OBJECT(x, name) ((x)->SetName(name))

