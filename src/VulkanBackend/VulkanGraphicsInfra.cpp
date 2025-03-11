#include "VulkanBackend/VulkanBackendPch.h"
#include "VulkanGraphicsInfra.h"

VulkanBackend::VulkanGraphicsInfra::VulkanGraphicsInfra()
{
}

VulkanBackend::VulkanGraphicsInfra::~VulkanGraphicsInfra()
{
}

std::unique_ptr<GI::IGraphicMemoryResource> VulkanBackend::VulkanGraphicsInfra::CreateMemoryResource(const GI::MemoryResourceDesc& desc)
{
	return std::unique_ptr<GI::IGraphicMemoryResource>();
}

void VulkanBackend::VulkanGraphicsInfra::InitialMemoryResourceFromImage(GI::IGraphicMemoryResource* resource, const GI::IImage& image)
{
}

void VulkanBackend::VulkanGraphicsInfra::CopyToUploadBufferResource(GI::IGraphicMemoryResource* resource, const std::span<const b8>& data)
{
}

std::unique_ptr<GI::IImage> VulkanBackend::VulkanGraphicsInfra::CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::span<const b8>& content, const char* name) const
{
	return std::unique_ptr<GI::IImage>();
}

void VulkanBackend::VulkanGraphicsInfra::AdaptToWindow(const Platform::WindowInfo& windowInfo, u8 frameCount)
{
}

void VulkanBackend::VulkanGraphicsInfra::ResizeWindow(Platform::NativeWindowHandle windowHandle, const Vec2u& windowSize)
{
}

GI::IGraphicMemoryResource* VulkanBackend::VulkanGraphicsInfra::GetWindowBackBuffer(Platform::NativeWindowHandle windowHandle)
{
	return nullptr;
}

void VulkanBackend::VulkanGraphicsInfra::StartFrame()
{
}

void VulkanBackend::VulkanGraphicsInfra::EndFrame()
{
}

void VulkanBackend::VulkanGraphicsInfra::Present()
{
}

void VulkanBackend::VulkanGraphicsInfra::StartRecording()
{
}

void VulkanBackend::VulkanGraphicsInfra::EndRecording(bool dropAllCommands)
{
}

GI::IGraphicsRecorder* VulkanBackend::VulkanGraphicsInfra::GetRecorder() const
{
	return nullptr;
}

GI::DevicePtr VulkanBackend::VulkanGraphicsInfra::GetNativeDevicePtr() const
{
	return GI::DevicePtr();
}
