#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "D3D12Headers.h"

namespace D3D12Backend
{
	class GD_D3D12BACKEND_API D3D12GraphicsInfra : public GI::IGraphicsInfra
	{
	public:
		D3D12GraphicsInfra();

		std::unique_ptr<GI::IGraphicMemoryResource> CreateMemoryResource(const GI::MemoryResourceDesc& desc) override;
		std::unique_ptr<GI::IGraphicMemoryResource> CreateMemoryResource(const GI::IImage& image) override;

		void CopyToUploadMemoryResource(GI::IGraphicMemoryResource* resource, const std::vector<b8>& data) override;

		std::unique_ptr<GI::IImage> CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::vector<b8>& content) const override;
		std::unique_ptr<GI::IImage> CreateFromScratch(GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name) const override;

		void                        AdaptToWindow(u8 windowId, const WindowRuntimeInfo& windowInfo) override;
		GI::IGraphicMemoryResource* GetWindowBackBuffer(u8 windowId) override;

		void                        StartFrame() override;
		void                        EndFrame() override;
		void                        Present() override;

		void						StartRecording() override;
		void						EndRecording() override;
		class GI::IGraphicsRecorder* GetRecorder() const override;

		GI::DevicePtr               GetNativeDevicePtr() const override;

	private:
		class D3D12Device*	mDevice = nullptr;
		class D3D12GraphicsRecorder* mCurrentRecorder = nullptr;
	};

	class GD_D3D12BACKEND_API D3D12GraphicsRecorder : public GI::IGraphicsRecorder
	{
	public:
		D3D12GraphicsRecorder(D3D12CommandContext* context);

		void AddClearOperation(const GI::RtvDesc& rtv, const Vec4f& value) override;
		void AddClearOperation(const GI::DsvDesc& dsv, bool clearDepth, float depth, bool clearStencil, u32 stencil) override;
		void AddCopyOperation(GI::IGraphicMemoryResource* dest, GI::IGraphicMemoryResource* src) override;
		void AddGraphicsPass(const GI::GraphicsPass& pass) override;
		void AddComputePass(const GI::ComputePass& pass) override;
		void AddPreparePresent(GI::IGraphicMemoryResource* res) override;
		void AddBeginEvent(const char* mark) override;
		void AddEndEvent() override;

		void Finalize();

		// TODO remove this
		D3D12CommandContext* GetContext() const { return mContext; }

	private:
		D3D12CommandContext* mContext = nullptr;
	};
}