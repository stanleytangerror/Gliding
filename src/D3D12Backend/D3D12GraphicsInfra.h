#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "D3D12Headers.h"

namespace D3D12Backend
{
	class GD_D3D12BACKEND_API D3D12GraphicsInfra : public GI::IGraphicsInfra
	{
	public:
		D3D12GraphicsInfra(class D3D12Device* device);

		std::unique_ptr<GI::IGraphicMemoryResource> CreateMemoryResource(const GI::MemoryResourceDesc& desc) override;
		std::unique_ptr<GI::IGraphicMemoryResource> CreateMemoryResource(const GI::IImage& image) override;

		void CopyToUploadMemoryResource(GI::IGraphicMemoryResource* resource, const std::vector<b8>& data) override;

		std::unique_ptr<GI::IImage> CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::vector<b8>& content) const override;
		std::unique_ptr<GI::IImage> CreateFromScratch(GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name) const override;

	private:
		class D3D12Device*	mDevice = nullptr;
		class D3D12CommandContext* mCurrentContext = nullptr;
	};
}