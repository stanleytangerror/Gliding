#pragma once

#include "D3D12Headers.h"

class D3D12CommandContext;

namespace D3D12Utils
{
	void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);
	
	ID3DBlob* LoadRs(const char* filePath, const char* entry);
	ID3DBlob* LoadVs(const char* filePath);
	ID3DBlob* LoadPs(const char* filePath);
	ID3DBlob* LoadCs(const char* filePath);

	ID3DBlob* CompileBlobFromFile(const char* filePath, const char* entryName, const char* target, u32 flags);

	ID3D12Resource* CreateUploadBuffer(ID3D12Device* device, u64 size, const char* name = nullptr);

	void SetRawD3D12ResourceName(ID3D12Resource* res, const char* name);
	void SetRawD3D12ResourceName(ID3D12Resource* res, const std::string& name);
	void SetRawD3D12ResourceName(ID3D12Resource* res, const wchar_t* name);
	void SetRawD3D12ResourceName(ID3D12Resource* res, const std::wstring& name);

	ID3D12Resource* CreateTextureFromImageFile(D3D12CommandContext* context, const char* filePath);
	ID3D12Resource* CreateTextureFromImageMemory(D3D12CommandContext* context, const char* filePath, const std::vector<b8>& content);
	
	D3D12_COMPARISON_FUNC ToDepthCompareFunc(const Math::ValueCompareState& state);

	class ShaderInclude : public ID3DInclude
	{
	public:
		ShaderInclude(const char* root);

		HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
		HRESULT Close(LPCVOID pData);
	protected:
		std::string const	mRoot;
		std::string	mContent;
	};
}

#define NAME_RAW_D3D12_OBJECT(x, name)	{if (name) { D3D12Utils::SetRawD3D12ResourceName((x), (name)); }}

