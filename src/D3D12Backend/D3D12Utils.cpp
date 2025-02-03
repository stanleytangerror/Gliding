#include "D3D12Backend/D3D12BackendPch.h"
#include "D3D12Utils.h"
#include "D3D12CommandContext.h"
#include "D3D12Resource.h"
#include <DirectXTex/DirectXTex.h>
#include <ranges>

// https://stackoverflow.com/a/60971856/2131563
namespace detail {
	// Type acts as a tag to find the correct operator| overload
	template <typename C>
	struct to_helper {};

	// This actually does the work
	template <typename Container, std::ranges::range R>
	requires std::convertible_to<std::ranges::range_value_t<R>, typename Container::value_type>
	Container operator|(R&& r, to_helper<Container>) 
	{
		return Container{ r.begin(), r.end() };
	}
}

// Couldn't find an concept for container, however a container is a range, but not a view.
template <std::ranges::range Container>
requires (!std::ranges::view<Container>)
auto to() 
{
	return detail::to_helper<Container>{};
}

D3D12_INPUT_ELEMENT_DESC D3D12Utils::ToD3D12InputElementDesc(const GI::InputElementDesc& desc)
{
	return {
		desc.GetSemanticName(),
		desc.GetSemanticIndex(),
		D3D12Utils::ToDxgiFormat(desc.GetFormat()),
		desc.GetInputSlot(),
		desc.GetAlignedByteOffset(),
		D3D12_INPUT_CLASSIFICATION(desc.GetInputSlotClass()),
		desc.GetInstanceDataStepRate()
	};
}

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

std::string GetContentFromD3DBlob(ID3DBlob* blob)
{
	Assert(blob);
	std::vector<char> buf(blob->GetBufferSize());
	std::memcpy(buf.data(), blob->GetBufferPointer(), blob->GetBufferSize());
	return std::string(buf.begin(), buf.end());
}

ID3DBlob* D3D12Utils::CompileBlobFromFile(const char* filePath, const char* entryName, const char* target, const std::vector<D3D_SHADER_MACRO>& macros)
{
	std::ostringstream buf;
	std::ifstream input(filePath);
	buf << input.rdbuf();
	
	const std::string& content = buf.str();
	std::unique_ptr< ShaderInclude> include = std::make_unique<ShaderInclude>(Utils::GetDirFromPath(filePath).c_str());

	ID3DBlob* result = nullptr;
	ID3DBlob* error = nullptr;

	u32 flags = 0;
#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_SKIP_VALIDATION;
#endif

	const HRESULT hr = D3DCompile(content.c_str(), content.size(), filePath, macros.data(), include.get(), entryName, target, flags, 0, &result, &error);
	if (FAILED(hr))
	{
		{
			ID3DBlob* preprocessCode = nullptr;
			ID3DBlob* preprocessError = nullptr;
			const HRESULT preprocessResult = D3DPreprocess(content.c_str(), content.size(), filePath, macros.data(), include.get(), &preprocessCode, &preprocessError);
			if (FAILED(preprocessResult))
			{
				Utils::PrintDebugString(GetContentFromD3DBlob(preprocessError).c_str());
				Assert(false);
			}
			Utils::PrintDebugString(GetContentFromD3DBlob(preprocessCode).c_str());
		}
		Utils::PrintDebugString(GetContentFromD3DBlob(error).c_str());
		AssertHResultOk(hr);
	}

	return result;
}

void D3D12Utils::SetRawD3D12ResourceName(ID3D12Object* res, const char* name)
{
	SetRawD3D12ResourceName(res, Utils::ToWString(name));
}

void D3D12Utils::SetRawD3D12ResourceName(ID3D12Object* res, const std::string& name)
{
	SetRawD3D12ResourceName(res, Utils::ToWString(name));
}

void D3D12Utils::SetRawD3D12ResourceName(ID3D12Object* res, const wchar_t* name)
{
	res->SetName(name);
}

void D3D12Utils::SetRawD3D12ResourceName(ID3D12Object* res, const std::wstring& name)
{
	SetRawD3D12ResourceName(res, name.c_str());
}

void D3D12Utils::InitialD3DResourceFromImage(D3D12Backend::D3D12CommandContext* context, GI::IGraphicMemoryResource* resource, const GI::IImage& image)
{
	D3D12Backend::D3D12Device* device = context->GetDevice();

	// https://github.com/microsoft/DirectXTex/wiki/CreateTexture
	auto resultDeviceResource = device->GetResourceManager()->GetResource(resource->GetDeviceResourceId());
	resultDeviceResource->Transition(context, D3D12_RESOURCE_STATE_COPY_DEST);

	const auto& subImages = image.GetImageContent().subImages;
	const auto& subresources = subImages
		| std::views::transform([](const GI::IImage::SubImageContent& subImage) { return D3D12_SUBRESOURCE_DATA{ subImage.pixels, LONG_PTR(subImage.rowPitch), LONG_PTR(subImage.slicePitch) }; })
		| to<std::vector<D3D12_SUBRESOURCE_DATA>>();

	// upload is implemented by application developer. Here's one solution using <d3dx12.h>
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resultDeviceResource->GetD3D12Resource(), 0, static_cast<unsigned int>(subresources.size()));

	auto textureUploadHeap = device->GetResourceManager()->CreateResource(GI::MemoryResourceDesc()
		.SetDimension(GI::ResourceDimension::BUFFER)
		.SetAlignment(0)
		.SetWidth(uploadBufferSize)
		.SetHeight(1)
		.SetDepthOrArraySize(1)
		.SetMipLevels(1)
		.SetFormat(GI::Format::FORMAT_UNKNOWN)
		.SetLayout(GI::TextureLayout::LAYOUT_ROW_MAJOR)
		.SetFlags(GI::ResourceFlag::NONE)
		.SetInitState(GI::ResourceState::STATE_GENERIC_READ)
		.SetHeapType(GI::HeapType::UPLOAD));
	// unresolved external symbol IID_ID3D12Device: https://github.com/microsoft/DirectX-Graphics-Samples/issues/567#issuecomment-525846757

	auto innerUploadResource = device->GetResourceManager()->GetResource(textureUploadHeap->GetDeviceResourceId());

	UpdateSubresources(
		context->GetCommandList(),
		resultDeviceResource->GetD3D12Resource(),
		innerUploadResource->GetD3D12Resource(),
		0, 0, static_cast<unsigned int>(subresources.size()),
		subresources.data());
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

DXGI_FORMAT D3D12Utils::GetBaseFormat(DXGI_FORMAT defaultFormat)
{
	switch (defaultFormat)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;

	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;

	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;

		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_R32G8X24_TYPELESS;

		// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_TYPELESS;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_R24G8_TYPELESS;

		// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_R16_TYPELESS;

	default:
		return defaultFormat;
	}
}

DXGI_FORMAT D3D12Utils::GetUAVFormat(DXGI_FORMAT defaultFormat)
{
	switch (defaultFormat)
	{
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM;

	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8X8_UNORM;

	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;

	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_D16_UNORM:
		Assert(false);
		return defaultFormat;

	default:
		return defaultFormat;
	}
}

DXGI_FORMAT D3D12Utils::GetDSVFormat(DXGI_FORMAT defaultFormat)
{
	switch (defaultFormat)
	{
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;

		// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_D16_UNORM;

	default:
		return defaultFormat;
	}
}

DXGI_FORMAT D3D12Utils::GetDepthFormat(DXGI_FORMAT defaultFormat)
{
	switch (defaultFormat)
	{
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

		// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT D3D12Utils::GetStencilFormat(DXGI_FORMAT defaultFormat)
{
	switch (defaultFormat)
	{
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

D3D12_SRV_DIMENSION D3D12Utils::GetSrvDimension(D3D12_RESOURCE_DIMENSION dim)
{
	switch (dim)
	{
	case D3D12_RESOURCE_DIMENSION_UNKNOWN: return D3D12_SRV_DIMENSION_UNKNOWN;
	case D3D12_RESOURCE_DIMENSION_BUFFER:return D3D12_SRV_DIMENSION_BUFFER;
	case D3D12_RESOURCE_DIMENSION_TEXTURE1D:return D3D12_SRV_DIMENSION_TEXTURE1D;
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:return D3D12_SRV_DIMENSION_TEXTURE2D;
	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:return D3D12_SRV_DIMENSION_TEXTURE3D;
	default:Assert(false); return D3D12_SRV_DIMENSION_UNKNOWN;
	}
}

D3D12_UAV_DIMENSION D3D12Utils::GetUavDimension(D3D12_RESOURCE_DIMENSION dim)
{
	switch (dim)
	{
	case D3D12_RESOURCE_DIMENSION_UNKNOWN: return D3D12_UAV_DIMENSION_UNKNOWN;
	case D3D12_RESOURCE_DIMENSION_BUFFER:return D3D12_UAV_DIMENSION_BUFFER;
	case D3D12_RESOURCE_DIMENSION_TEXTURE1D:return D3D12_UAV_DIMENSION_TEXTURE1D;
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:return D3D12_UAV_DIMENSION_TEXTURE2D;
	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:return D3D12_UAV_DIMENSION_TEXTURE3D;
	default:Assert(false); return D3D12_UAV_DIMENSION_UNKNOWN;
	}
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

