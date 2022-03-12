#pragma once

#include "D3D12Headers.h"
#include <DirectXTex/DirectXTex.h>

namespace D3D12Utils
{
	void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);
	
	ID3DBlob* LoadRs(const char* filePath, const char* entry);
	ID3DBlob* LoadVs(const char* filePath);
	ID3DBlob* LoadPs(const char* filePath);
	ID3DBlob* LoadCs(const char* filePath);

	ID3DBlob* CompileBlobFromFile(const char* filePath, const char* entryName, const char* target, u32 flags);
	ID3DBlob* CompileBlob(const void* date, const i32 dataSize, const char* entryName, const char* target, u32 flags);

	ID3D12Resource* CreateUploadBuffer(ID3D12Device* device, u64 size, const char* name = nullptr);

	void SetRawD3D12ResourceName(ID3D12Resource* res, const char* name);
	void SetRawD3D12ResourceName(ID3D12Resource* res, const std::string& name);
	void SetRawD3D12ResourceName(ID3D12Resource* res, const wchar_t* name);
	void SetRawD3D12ResourceName(ID3D12Resource* res, const std::wstring& name);

	std::unique_ptr<DirectX::ScratchImage> LoadDDSImageFromFile(const char* filePath);
	std::pair<ID3D12Resource*, ID3D12Resource*> CreateD3DResFromDDSImage(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const DirectX::ScratchImage& image);
}

#define NAME_RAW_D3D12_OBJECT(x, name)	{if (name) { D3D12Utils::SetRawD3D12ResourceName((x), (name)); }}

