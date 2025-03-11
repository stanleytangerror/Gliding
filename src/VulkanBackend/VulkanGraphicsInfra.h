#pragma once

#include "Common/GraphicsInfrastructure.h"

namespace VulkanBackend
{
	class VulkanGraphicsInfra : public GI::IGraphicsInfra
	{
	public:
		VulkanGraphicsInfra();
		~VulkanGraphicsInfra() override;
		std::unique_ptr<GI::IGraphicMemoryResource> CreateMemoryResource(const GI::MemoryResourceDesc& desc) override;
		void InitialMemoryResourceFromImage(GI::IGraphicMemoryResource* resource, const GI::IImage& image) override;
		void CopyToUploadBufferResource(GI::IGraphicMemoryResource* resource, const std::span<const b8>& data) override;
		std::unique_ptr<GI::IImage> CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::span<const b8>& content, const char* name) const override;
		void                        AdaptToWindow(const Platform::WindowInfo& windowInfo, u8 frameCount) override;
		void                        ResizeWindow(Platform::NativeWindowHandle windowHandle, const Vec2u& windowSize) override;
		GI::IGraphicMemoryResource* GetWindowBackBuffer(Platform::NativeWindowHandle windowHandle) override;
		void                        StartFrame() override;
		void                        EndFrame() override;
		void                        Present() override;
		void						StartRecording() override;
		void						EndRecording(bool dropAllCommands) override;
		class GI::IGraphicsRecorder* GetRecorder() const override;
		GI::DevicePtr               GetNativeDevicePtr() const override;
	private:
		class VulkanDevice* mDevice = nullptr;
		class VulkanGraphicsRecorder* mCurrentRecorder = nullptr;
		bool						mSkipFrameCommands = false;
	};
}

extern "C"
{
	GD_VULKANBACKEND_API GI::IGraphicsInfra* CreateGraphicsInfra();
}