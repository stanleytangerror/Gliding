#include "Common/GraphicsInfrastructure.h"
#include "DirectXTex/DirectXTex.h"

namespace D3D12Backend
{
	class WindowsImage : public GI::IImage
	{
	public:
		static std::unique_ptr<WindowsImage> CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::span<const b8>& content, const char* name);

		WindowsImage(std::unique_ptr<DirectX::ScratchImage>&& image, const char* name);

		DirectX::ScratchImage* GetImage() const { return mImage.get(); }
		const char* GetName() const { return mName.c_str(); }

		GI::MemoryResourceDesc GetResourceDesc() const override;
		GI::IImage::ImageContent GetImageContent() const override;

	protected:
		const std::unique_ptr<DirectX::ScratchImage> mImage;
		const std::string mName;
	};
}