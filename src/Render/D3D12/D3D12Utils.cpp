#include "RenderPch.h"
#include "D3D12Utils.h"
#include "D3D12CommandContext.h"
#include <DirectXTex/DirectXTex.h>

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
	std::unique_ptr< ShaderInclude> include = std::make_unique<ShaderInclude>(Utils::GetDirFromPath(filePath).c_str());

	ID3DBlob* result = nullptr;
	ID3DBlob* error = nullptr;

#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_SKIP_VALIDATION;
#endif

	const HRESULT hr = D3DCompile(content.c_str(), content.size(), nullptr, nullptr, include.get(), entryName, target, flags, 0, &result, &error);
	if (hr)
	{
		char errorStr[1024] = {};
		std::memcpy(errorStr, (char*)error->GetBufferPointer(), error->GetBufferSize());
		OutputDebugString(errorStr);
		AssertHResultOk(hr);
	}

	return result;
}

ID3D12Resource* D3D12Utils::CreateUploadBuffer(ID3D12Device* device, u64 size, const char* name)
{
	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

	ID3D12Resource* result = nullptr;

	AssertHResultOk(device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&bufDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&result)));

	NAME_RAW_D3D12_OBJECT(result, name);

	return result;
}

void D3D12Utils::SetRawD3D12ResourceName(ID3D12Resource* res, const char* name)
{
	SetRawD3D12ResourceName(res, Utils::ToWString(name));
}

void D3D12Utils::SetRawD3D12ResourceName(ID3D12Resource* res, const std::string& name)
{
	SetRawD3D12ResourceName(res, Utils::ToWString(name));
}

void D3D12Utils::SetRawD3D12ResourceName(ID3D12Resource* res, const wchar_t* name)
{
	res->SetName(name);
}

void D3D12Utils::SetRawD3D12ResourceName(ID3D12Resource* res, const std::wstring& name)
{
	SetRawD3D12ResourceName(res, name.c_str());
}

namespace
{
	std::unique_ptr<DirectX::ScratchImage> LoadDDSImageFromFile(const char* filePath)
	{
		// https://github.com/microsoft/DirectXTex/wiki/DDS-I-O-Functions
		DirectX::TexMetadata metadata;
		auto image = std::make_unique<DirectX::ScratchImage>();
		AssertHResultOk(DirectX::LoadFromDDSFile(Utils::ToWString(filePath).c_str(), DirectX::DDS_FLAGS_NONE, &metadata, *image));
		return image;
	}

	std::unique_ptr<DirectX::ScratchImage> LoadSpecificFormatImageFromFile_PngBmpGifTiffJpeg(const char* filePath)
	{
		DirectX::TexMetadata metadata;
		auto image = std::make_unique<DirectX::ScratchImage>();
		AssertHResultOk(DirectX::LoadFromWICFile(Utils::ToWString(filePath).c_str(), DirectX::WIC_FLAGS_NONE, &metadata, *image));
		return image;
	}

	std::pair<ID3D12Resource*, ID3D12Resource*> CreateD3DResFromScratchImage(D3D12CommandContext* context, const DirectX::ScratchImage& image)
	{
		ID3D12Device* device = context->GetDevice()->GetDevice();
		ID3D12GraphicsCommandList* commandList = context->GetCommandList();

		// https://github.com/microsoft/DirectXTex/wiki/CreateTexture
		ID3D12Resource* result = nullptr;
		AssertHResultOk(DirectX::CreateTexture(device, image.GetMetadata(), &result));

		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		AssertHResultOk(DirectX::PrepareUpload(device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), subresources));

		// upload is implemented by application developer. Here's one solution using <d3dx12.h>
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(result, 0, static_cast<unsigned int>(subresources.size()));

		ID3D12Resource* textureUploadHeap = nullptr;
		const CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		AssertHResultOk(device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureUploadHeap)));
		// unresolved external symbol IID_ID3D12Device: https://github.com/microsoft/DirectX-Graphics-Samples/issues/567#issuecomment-525846757

		UpdateSubresources(commandList,
			result, textureUploadHeap,
			0, 0, static_cast<unsigned int>(subresources.size()),
			subresources.data());

		return { result, textureUploadHeap };
	}
}

ID3D12Resource* D3D12Utils::CreateTextureFromImageFile(D3D12CommandContext* context, const char* filePath)
{
	std::filesystem::path ext = std::filesystem::path(filePath).extension();
	std::unique_ptr<DirectX::ScratchImage> image;
	if (ext == ".dds")
	{
		image = LoadDDSImageFromFile(filePath);
	}
	else if (ext == ".png" || ext == ".bmp" || ext == ".gif" || ext == ".tiff" || ext == ".jpeg" || ext == ".jpg")
	{
		image = LoadSpecificFormatImageFromFile_PngBmpGifTiffJpeg(filePath);
	}

	if (image->GetImageCount() != 0)
	{
		const auto& result = CreateD3DResFromScratchImage(context, *image);
		ID3D12Resource* resource = result.first;
		ID3D12Resource* tempRes = result.second;
		NAME_RAW_D3D12_OBJECT(resource, filePath);
		NAME_RAW_D3D12_OBJECT(tempRes, "IntermediateHeap");
		context->GetDevice()->ReleaseD3D12Resource(tempRes);

		return resource;
	}

	Assert(false);
	return nullptr;
}

D3D12_COMPARISON_FUNC D3D12Utils::ToDepthCompareFunc(const Math::ValueCompareState& state)
{
	if (state == Math::ValueCompareState_Equal) return D3D12_COMPARISON_FUNC_EQUAL;
	if (state == Math::ValueCompareState_Less) return D3D12_COMPARISON_FUNC_LESS;
	if (state == Math::ValueCompareState_Greater) return D3D12_COMPARISON_FUNC_GREATER;
	if (state == (Math::ValueCompareState_Equal | Math::ValueCompareState_Less)) return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	if (state == (Math::ValueCompareState_Equal | Math::ValueCompareState_Greater)) return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	if (state == (Math::ValueCompareState_Less | Math::ValueCompareState_Greater)) return D3D12_COMPARISON_FUNC_NOT_EQUAL;

	Assert(false);
	return D3D12_COMPARISON_FUNC_ALWAYS;
}

D3D12Utils::ShaderInclude::ShaderInclude(const char* root)
	: mRoot(std::string(root) + "/")
{

}

HRESULT D3D12Utils::ShaderInclude::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	const std::string& filePath = mRoot + pFileName;
	std::ifstream input(filePath.c_str());
	if (!input.is_open())
	{
		Assert(false);
		return E_FAIL;
	}

	std::ostringstream buf;
	buf << input.rdbuf();

	mContent = buf.str();
	*ppData = mContent.data();
	*pBytes = static_cast<u32>(mContent.size());

	return S_OK;
}

HRESULT D3D12Utils::ShaderInclude::Close(LPCVOID pData)
{
	mContent.clear();
	return S_OK;
}

