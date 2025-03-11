#include "D3D12Backend/D3D12BackendPch.h"
#include "WinImage.h"
#include "D3D12Utils.h"

namespace D3D12Backend
{
	WindowsImage::WindowsImage(const std::span<const b8>& content, std::unique_ptr<DirectX::ScratchImage>&& image, const char* name)
		: mRawContent(content.begin(), content.end())
		, mImage(std::forward<std::unique_ptr<DirectX::ScratchImage>>(image))
		, mName(name)
	{}

	GI::MemoryResourceDesc WindowsImage::GetResourceDesc() const
	{
		// DirectXTexD3D12.cpp DirectX::CreateTextureEx

		const auto& metadata = mImage->GetMetadata();

		Assert(metadata.mipLevels);
		Assert(metadata.arraySize);
		Assert(metadata.width <= UINT32_MAX);
		Assert(metadata.height <= UINT32_MAX);
		Assert(metadata.mipLevels <= UINT16_MAX);
		Assert(metadata.arraySize <= UINT16_MAX);

		return GI::MemoryResourceDesc()
			.SetWidth(static_cast<UINT>(metadata.width))
			.SetHeight(static_cast<UINT>(metadata.height))
			.SetMipLevels(static_cast<UINT16>(metadata.mipLevels))
			.SetDepthOrArraySize((metadata.dimension == GI::ResourceDimension::TEXTURE3D)
				? static_cast<UINT16>(metadata.depth)
				: static_cast<UINT16>(metadata.arraySize))
			.SetFormat(D3D12Utils::ToGiFormat(metadata.format))
			.SetFlags(GI::ResourceFlag::NONE)
			.SetSampleDesc_Count(1)
			.SetDimension(static_cast<GI::ResourceDimension::Enum>(metadata.dimension))
			.SetHeapType(GI::HeapType::DEFAULT)
			.SetName(mName.c_str());
	}


	GI::IImage::ImageContent WindowsImage::GetImageContent() const
	{
		// DirectXTexD3D12.cpp DirectX::PrepareUpload

		ImageContent content;

		const auto& metadata = mImage->GetMetadata();
		content.dimension = GI::ResourceDimension::Enum(metadata.dimension);

		switch (metadata.dimension)
		{
		case DirectX::TEX_DIMENSION_TEXTURE2D:
			for (auto item = 0; item < metadata.arraySize; ++item)
			{
				for (auto mip = 0; mip < metadata.mipLevels; ++mip)
				{
					const auto& subImage = mImage->GetImage(mip, item, 0);
					Assert(subImage->format == metadata.format);
					Assert(subImage->pixels);

					content.subImages.emplace_back((b8*)subImage->pixels, u64(subImage->rowPitch), u64(subImage->slicePitch));
				}
			}
			break;
		default:
			Assert(false);
			break;
		}

		return content;
	}

	std::unique_ptr<WindowsImage> WindowsImage::CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::span<const b8>& content, const char* name)
	{
		switch (ext)
		{
		case TextureFileExt::DDS:
		{
			// https://github.com/microsoft/DirectXTex/wiki/DDS-I-O-Functions
			auto image = std::make_unique<DirectX::ScratchImage>();
			auto hr = DirectX::LoadFromDDSMemory(content.data(), content.size(), DirectX::DDS_FLAGS_NONE, nullptr, *image);
			AssertHResultOk(hr);
			return std::make_unique<WindowsImage>(content, std::move(image), name);
		}
		case TextureFileExt::PNG:
		case TextureFileExt::BMP:
		case TextureFileExt::GIF:
		case TextureFileExt::TIFF:
		case TextureFileExt::JPEG:
		case TextureFileExt::JPG:
		{
			auto image = std::make_unique<DirectX::ScratchImage>();
			auto hr = DirectX::LoadFromWICMemory(content.data(), content.size(), DirectX::WIC_FLAGS_NONE, nullptr, *image);
			AssertHResultOk(hr);
			return std::make_unique<WindowsImage>(content, std::move(image), name);
		}
		default:
			Assert(false);
			return nullptr;
		}
	}
}