#pragma once

#include "D3D12Headers.h"
#include "Common/Texture.h"
#include "Common/GraphicsInfrastructure.h"
#include "DirectXTex/DirectXTex.h"

namespace D3D12Backend
{
	class D3D12CommandContext;
	class CommitedResource;
}

namespace DirectX
{
	class ScratchImage;
}

namespace D3D12Utils
{
	class WindowsImage;

	constexpr DXGI_FORMAT ToDxgiFormat(GI::Format::Enum format) { return DXGI_FORMAT(format); }
	constexpr GI::Format::Enum ToGiFormat(DXGI_FORMAT format) { return GI::Format::Enum(format); }

	D3D12_INPUT_ELEMENT_DESC ToD3D12InputElementDesc(const GI::InputElementDesc& desc);

	void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);
	
	ID3DBlob* CompileBlobFromFile(const char* filePath, const char* entryName, const char* target, const std::vector<D3D_SHADER_MACRO>& macros);

	GD_D3D12BACKEND_API void SetRawD3D12ResourceName(ID3D12Object* res, const char* name);
	GD_D3D12BACKEND_API void SetRawD3D12ResourceName(ID3D12Object* res, const std::string& name);
	GD_D3D12BACKEND_API void SetRawD3D12ResourceName(ID3D12Object* res, const wchar_t* name);
	GD_D3D12BACKEND_API void SetRawD3D12ResourceName(ID3D12Object* res, const std::wstring& name);

	//GD_D3D12BACKEND_API std::unique_ptr<D3D12Backend::CommitedResource> CreateTextureFromImageFile(D3D12Backend::D3D12CommandContext* context, const char* filePath);
	GD_D3D12BACKEND_API std::unique_ptr<GI::IGraphicMemoryResource> CreateTextureFromImageMemory(D3D12Backend::D3D12CommandContext* context, const TextureFileExt::Enum& ext, const std::vector<b8>& content);
	GD_D3D12BACKEND_API std::unique_ptr<GI::IGraphicMemoryResource> CreateTextureFromRawMemory(D3D12Backend::D3D12CommandContext* context, DXGI_FORMAT format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name);
	GD_D3D12BACKEND_API std::unique_ptr<GI::IGraphicMemoryResource> CreateResourceFromImage(D3D12Backend::D3D12CommandContext* context, const D3D12Utils::WindowsImage& image);
	
	GD_D3D12BACKEND_API D3D12_COMPARISON_FUNC ToDepthCompareFunc(const Math::ValueCompareState& state);

	/* dxgi format util functions from Microsoft/DirectX-Graphics-Samples */
	DXGI_FORMAT GetBaseFormat(DXGI_FORMAT defaultFormat);
	DXGI_FORMAT GetUAVFormat(DXGI_FORMAT defaultFormat);
	DXGI_FORMAT GetDSVFormat(DXGI_FORMAT defaultFormat);
	DXGI_FORMAT GetDepthFormat(DXGI_FORMAT defaultFormat);
	DXGI_FORMAT GetStencilFormat(DXGI_FORMAT defaultFormat);

	GD_D3D12BACKEND_API D3D12_SRV_DIMENSION GetSrvDimension(D3D12_RESOURCE_DIMENSION dim);
	GD_D3D12BACKEND_API D3D12_UAV_DIMENSION GetUavDimension(D3D12_RESOURCE_DIMENSION dim);

	template <typename T>
	inline std::vector<byte> ToD3DConstBufferParamData(const T& var);

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

	class WindowsImage : public GI::IImage
	{
	public:
		static std::unique_ptr<WindowsImage> CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::vector<b8>& content);
		static std::unique_ptr<WindowsImage> CreateFromScratch(GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name);

		WindowsImage(std::unique_ptr<DirectX::ScratchImage>&& image);

		DirectX::ScratchImage* GetImage() const { return mImage.get(); }

	protected:
		const std::unique_ptr<DirectX::ScratchImage> mImage;
	};
}

#define NAME_RAW_D3D12_OBJECT(x, name)	{if (name) { D3D12Utils::SetRawD3D12ResourceName((x), (name)); }}

#include "D3D12Utils_inl.h"

