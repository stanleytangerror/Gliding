#include "RenderPch.h"
#include "D3D12Utils.h"
#include "Common/AssertUtils.h"

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
			// If you want a software adapter, pass in "/warp" on the command line.
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

ID3DBlob* D3D12Utils::LoadRs(const wchar_t* file, const char* entry)
{
	ID3DBlob* rootSignature = nullptr;
	ID3DBlob* error = nullptr;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	HRESULT hr = D3DCompileFromFile(file, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, "rootsig_1_0", compileFlags, 0, &rootSignature, &error);
	if (FAILED(hr))
	{
		char errorStr[1024] = {};
		memcpy(errorStr, error->GetBufferPointer(), error->GetBufferSize());
		OutputDebugString(errorStr);
		AssertHResultOk(hr);
	}

	return rootSignature;
}