#include "D3D12BackendPch.h"
#include "D3D12GraphicsInfra.h"
#include "D3D12Resource.h"
#include "D3D12SwapChain.h"
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
		const auto& dxScratchImage = *(reinterpret_cast<const D3D12Utils::WindowsImage*>(&image));
		return D3D12Utils::CreateResourceFromImage(mCurrentRecorder->GetContext(), dxScratchImage);
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
		return std::move(D3D12Utils::WindowsImage::CreateFromImageMemory(ext, content));
	}

	std::unique_ptr<GI::IImage> D3D12GraphicsInfra::CreateFromScratch(GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name) const
	{
		return std::move(D3D12Utils::WindowsImage::CreateFromScratch(format, content, size, mipLevel, name));
	}


	void D3D12GraphicsInfra::AdaptToWindow(u8 windowId, const WindowRuntimeInfo& windowInfo)
	{
		mDevice->CreateSwapChain(PresentPortType(windowId), HWND(windowInfo.mNativeHandle), windowInfo.mSize);
	}

	GI::RtvDesc D3D12GraphicsInfra::GetWindowBackBufferRtv(u8 windowId)
	{
		mDevice->GetSwapChainBuffers(PresentPortType(windowId))->GetBuffer()->GetRtv();
	}

	void D3D12GraphicsInfra::StartFrame()
	{
		mDevice->StartFrame();
	}

	void D3D12GraphicsInfra::Present()
	{
		mDevice->Present();
	}

	void D3D12GraphicsInfra::StartRecording()
	{
		auto context = mDevice->GetGpuQueue(D3D12Backend::D3D12GpuQueueType::Graphic)->AllocGraphicContext();
		mCurrentRecorder = new class D3D12GraphicsRecorder(context);
	}

	void D3D12GraphicsInfra::EndRecording()
	{
		mCurrentRecorder->Finalize();
		Utils::SafeDelete(mCurrentRecorder);
	}

	GI::IGraphicsRecorder* D3D12GraphicsInfra::GetRecorder() const
	{
		return mCurrentRecorder;
	}


	GI::DevicePtr D3D12GraphicsInfra::GetNativeDevicePtr() const
	{
		return mDevice->GetDevice();
	}

	//////////////////////////////////////////////////////////////////////////

	D3D12GraphicsRecorder::D3D12GraphicsRecorder(D3D12CommandContext* context)
		: mContext(context)
	{

	}

	void D3D12GraphicsRecorder::AddClearOperation(const GI::RtvDesc& rtv, const Vec4f& value)
	{
		mContext->GetCommandList()->ClearRenderTargetView()
		throw std::logic_error("The method or operation is not implemented.");
	}

	void D3D12GraphicsRecorder::AddClearOperation(const GI::DsvDesc& dsv, float depth, u32 stencil)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void D3D12GraphicsRecorder::AddGraphicsPass(const GI::GraphicsPass& pass)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void D3D12GraphicsRecorder::AddComputePass(const GI::ComputePass& pass)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void D3D12GraphicsRecorder::Finalize()
	{
		mContext->Finalize();
	}

}