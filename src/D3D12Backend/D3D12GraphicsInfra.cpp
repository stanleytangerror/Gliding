#include "D3D12BackendPch.h"
#include "D3D12GraphicsInfra.h"
#include "D3D12Resource.h"
#include "Common/GraphicsInfrastructure.h"

namespace D3D12Backend
{
	D3D12GraphicsInfra::D3D12GraphicsInfra(class D3D12Device* device)
		: mDevice(device)
	{}

	std::unique_ptr<GI::IGraphicMemoryResource> D3D12GraphicsInfra::CreateMemoryResource(const GI::MemoryResourceDesc& desc)
	{
		return std::unique_ptr<GI::IGraphicMemoryResource>(
			D3D12Backend::CommitedResource::Builder()
			.SetDimention(D3D12_RESOURCE_DIMENSION(desc.GetDimension()))
			.SetFormat(D3D12Utils::ToDxgiFormat(desc.GetFormat()))
			.SetMipLevels(desc.GetMipLevels())
			.SetWidth(desc.GetWidth())
			.SetHeight(desc.GetHeight())
			.SetDepthOrArraySize(desc.GetDepthOrArraySize())
			.SetName(desc.GetName())
			.SetLayout(D3D12_TEXTURE_LAYOUT(desc.GetLayout()))
			.SetFlags(D3D12_RESOURCE_FLAGS(desc.GetFlags()))
			.SetInitState(D3D12_RESOURCE_STATES(desc.GetInitState()))
			.Build(mDevice, desc.GetHeapType()));
	}

	std::unique_ptr<GI::IGraphicMemoryResource> D3D12GraphicsInfra::CreateMemoryResource(const GI::IImage& image)
	{
		const auto& dxScratchImage = *(reinterpret_cast<const D3D12Utils::ScratchImage*>(&image));
		return D3D12Utils::CreateResourceFromImage(mCurrentContext, dxScratchImage);
	}

	void D3D12GraphicsInfra::CopyToUploadMemoryResource(GI::IGraphicMemoryResource* resource, const std::vector<b8>& data)
	{
		auto dx12Res = reinterpret_cast<D3D12Backend::CommitedResource*>(resource);
		u8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		AssertHResultOk(dx12Res->GetD3D12Resource()->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, data.data(), data.size());
	}

	std::unique_ptr<GI::IImage> D3D12GraphicsInfra::CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::vector<b8>& content) const
	{
		return std::move(D3D12Utils::ScratchImage::CreateFromImageMemory(ext, content));
	}

	std::unique_ptr<GI::IImage> D3D12GraphicsInfra::CreateFromScratch(GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name) const
	{
		return std::move(D3D12Utils::ScratchImage::CreateFromScratch(format, content, size, mipLevel, name));
	}

}