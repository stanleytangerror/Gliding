#include "RenderPch.h"
#include "D3D12Utils.h"
#include "Common/AssertUtils.h"
#include <fstream>
#include <sstream>

void D3D12Utils::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter)
{
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.Detach();
}

ID3DBlob* D3D12Utils::LoadRs(const char* file, const char* entry)
{
	return CompileBlobFromFile(file, entry, "rootsig_1_0", 0);
}

ID3DBlob* D3D12Utils::LoadVs(const char* file)
{
	return CompileBlobFromFile(file, "VSMain", "vs_5_0", 0);
}

ID3DBlob* D3D12Utils::LoadCs(const char* file)
{
	return CompileBlobFromFile(file, "CSMain", "cs_5_0", 0);
}

ID3DBlob* D3D12Utils::LoadPs(const char* file)
{
	return CompileBlobFromFile(file, "PSMain", "ps_5_0", 0);
}

ID3DBlob* D3D12Utils::CompileBlobFromFile(const char* filePath, const char* entryName, const char* target, u32 flags)
{
	std::ostringstream buf;
	std::ifstream input(filePath);
	buf << input.rdbuf();
	
	const std::string& content = buf.str();
	return CompileBlob(content.c_str(), content.size(), entryName, target, flags);
}

ID3DBlob* D3D12Utils::CompileBlob(const void* date, const i32 dataSize, const char* entryName, const char* target, u32 flags)
{
	ID3DBlob* result = nullptr;
	ID3DBlob* error = nullptr;

#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_SKIP_VALIDATION;
#endif

	std::string content;
	const HRESULT hr = D3DCompile(date, dataSize, nullptr, nullptr, nullptr, entryName, target, flags, 0, &result, &error);
	if (hr)
	{
		char errorStr[1024] = {};
		std::memcpy(errorStr, (char*)error->GetBufferPointer(), error->GetBufferSize());
		OutputDebugString(errorStr);
		AssertHResultOk(hr);
	}

	return result;
}