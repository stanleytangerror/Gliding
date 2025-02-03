#include "D3D12Backend/D3D12BackendPch.h"
#include "D3D12GraphicsInfra.h"
#include "D3D12Resource.h"
#include "D3D12SwapChain.h"
#include "Common/GraphicsInfrastructure.h"
#include "WinImage.h"
#include "../packages/WinPixEventRuntime.1.0.231030001/Include/WinPixEventRuntime/pix3.h"
#include <functional>
#include <ranges>

using namespace std::ranges;
using namespace std::ranges::views;

// https://brevzin.github.io/c++/2022/12/05/enumerate/
// https://www.reedbeta.com/blog/python-like-enumerate-in-cpp17/

template <typename T,
	typename TIter = decltype(std::begin(std::declval<T>())),
	typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T&& iterable)
{
	struct iterator
	{
		size_t i;
		TIter iter;
		bool operator != (const iterator& other) const { return iter != other.iter; }
		void operator ++ () { ++i; ++iter; }
		auto operator * () const { return std::tie(i, *iter); }
	};
	struct iterable_wrapper
	{
		T iterable;
		auto begin() { return iterator{ 0, std::begin(iterable) }; }
		auto end() { return iterator{ 0, std::end(iterable) }; }
	};
	return iterable_wrapper{ std::forward<T>(iterable) };
}

namespace D3D12Backend
{
	D3D12GraphicsInfra::D3D12GraphicsInfra()
	{
		mDevice = new D3D12Device;
	}


	D3D12GraphicsInfra::~D3D12GraphicsInfra()
	{
		mDevice->Destroy();
		Utils::SafeDelete(mDevice);
	}

	std::unique_ptr<GI::IGraphicMemoryResource> D3D12GraphicsInfra::CreateMemoryResource(const GI::MemoryResourceDesc& desc)
	{
		return std::move(mDevice->GetResourceManager()->CreateResource(desc));
	}

	void D3D12GraphicsInfra::InitialMemoryResourceFromImage(GI::IGraphicMemoryResource* resource, const GI::IImage& image)
	{
		return D3D12Utils::InitialD3DResourceFromImage(mCurrentRecorder->GetContext(), resource, image);
	}

	void D3D12GraphicsInfra::CopyToUploadBufferResource(GI::IGraphicMemoryResource* resource, const std::span<const b8>& data)
	{
		Assert(resource->GetDimension() == GI::ResourceDimension::BUFFER);
		Assert(resource->GetSize().x() >= data.size());

		auto dx12Res = mDevice->GetResourceManager()->GetResource(resource->GetResourceId());
		u8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		AssertHResultOk(dx12Res->GetD3D12Resource()->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, data.data(), data.size());
	}

	std::unique_ptr<GI::IImage> D3D12GraphicsInfra::CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::vector<b8>& content, const char* name) const
	{
		return std::move(WindowsImage::CreateFromImageMemory(ext, content, name));
	}


	void D3D12GraphicsInfra::AdaptToWindow(const Platform::WindowInfo& windowInfo, u8 frameCount)
	{
		mDevice->GetGpuQueue(D3D12GpuQueueType::Graphic)->CreateSwapChain(windowInfo.mNativeHandle, windowInfo.mSize, frameCount);
	}


	void D3D12GraphicsInfra::ResizeWindow(Platform::NativeWindowHandle windowHandle, const Vec2u& windowSize)
	{
		auto swapChain = mDevice->GetGpuQueue(D3D12GpuQueueType::Graphic)->GetSwapChain(windowHandle);

		if (swapChain->GetSize() != windowSize)
		{
			mDevice->PushPostSyncOperation(D3D12Device::PreRelease, 
				[swapChain, windowSize]()
				{
					swapChain->ClearBuffers();
				});

			mDevice->PushPostSyncOperation(D3D12Device::PostRelease, 
				[swapChain, windowSize]()
				{
					swapChain->Resize(windowSize);
				});

			mSkipFrameCommands = true;
		}
	}

	GI::IGraphicMemoryResource* D3D12GraphicsInfra::GetWindowBackBuffer(Platform::NativeWindowHandle windowHandle)
	{
		return mDevice->GetGpuQueue(D3D12GpuQueueType::Graphic)->GetSwapChain(windowHandle)->GetBuffer();
	}

	void D3D12GraphicsInfra::StartFrame()
	{
		mDevice->StartFrame();
		StartRecording();
	}

	void D3D12GraphicsInfra::EndFrame()
	{
		for (auto swapChain : mDevice->GetGpuQueue(D3D12GpuQueueType::Graphic)->GetSwapChains())
		{
			mCurrentRecorder->AddPreparePresent(swapChain->GetBuffer());
		}

		EndRecording(mSkipFrameCommands);
		mSkipFrameCommands = false;
	}

	void D3D12GraphicsInfra::Present()
	{
		mDevice->Present();
	}

	void D3D12GraphicsInfra::StartRecording()
	{
		auto context = mDevice->GetGpuQueue(D3D12Backend::D3D12GpuQueueType::Graphic)->AllocGraphicContext();
		mCurrentRecorder = new D3D12GraphicsRecorder(context);
	}

	void D3D12GraphicsInfra::EndRecording(bool dropAllCommands)
	{
		mCurrentRecorder->Finalize(dropAllCommands);
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

	namespace
	{
		i32 BindConstBufferParams(std::vector<b8>& cbuf, const std::map<std::string, StackMemory<64>>& cbArgs, ShaderPiece* shader)
		{
			const std::vector<InputCBufferParam>& cbufBindings = shader->GetCBufferBindings();
			const i32 cbSize = std::accumulate(cbufBindings.begin(), cbufBindings.end(), 0,
				[](i32 size, const auto& param) { return size + param.mSize; });

			i32 offset = cbuf.size();
			cbuf.insert(cbuf.end(), cbSize, {});
			for (const InputCBufferParam& cbParamStruct : shader->GetCBufferBindings())
			{
				for (const auto& [varName, varDesc] : cbParamStruct.mVariables)
				{
					if (cbArgs.find(varName) != cbArgs.end())
					{
						const auto& varArg = cbArgs.find(varName)->second;
						Assert(varDesc.mSize == varArg.GetSize());
						memcpy_s(cbuf.data() + varDesc.mStartOffset, varDesc.mSize, varArg.GetMemory(), varArg.GetSize());
					}
				}

				offset += cbParamStruct.mSize;
			}
			return cbSize;
		};

		template <typename T, typename V>
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> BindSrvUavParams(D3D12Backend::D3D12CommandContext* context, const std::map<std::string, T>& paramBindings, const std::map<std::string, V>& paramValues, const DescriptorPtr& nullDesc)
		{
			int maxIdx = 0;
			for (const auto& p : paramBindings)
			{
				const T& param = p.second;
				maxIdx = std::max<int>(maxIdx, param.mBindPoint);
			}

			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(maxIdx + 1, nullDesc.Get());
			for (const auto& p : paramBindings)
			{
				const std::string& name = p.first;
				const T& param = p.second;

				if (paramValues.find(name) != paramValues.end())
				{
					handles[param.mBindPoint] = paramValues.find(name)->second.Get();
				}
			}
			return handles;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	D3D12GraphicsRecorder::D3D12GraphicsRecorder(D3D12CommandContext* context)
		: mContext(context)
	{

	}

	void D3D12GraphicsRecorder::AddClearOperation(const GI::RtvUsage& rtv, const Vec4f& value)
	{
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();
		auto resId = rtv.GetResourceId();

		resourceManager->GetResource(resId)->Transition(mContext, D3D12_RESOURCE_STATE_RENDER_TARGET);

		const auto& descriptor = mContext->GetDevice()->GetResourceManager()->CreateRtvDescriptor(rtv.GetResourceId(), rtv.GetUsage());
		float rgba[4] = { value.x(), value.y(), value.z(), value.w() };
		mContext->GetCommandList()->ClearRenderTargetView(descriptor.Get(), rgba, 0, nullptr);
	}

	void D3D12GraphicsRecorder::AddClearOperation(const GI::DsvUsage& dsv, bool clearDepth, float depth, bool clearStencil, u32 stencil)
	{
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();
		auto resId = dsv.GetResourceId();

		resourceManager->GetResource(resId)->Transition(mContext, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		const auto& descriptor = mContext->GetDevice()->GetResourceManager()->CreateDsvDescriptor(dsv.GetResourceId(), dsv.GetUsage());
		auto flag =
			(clearDepth ? D3D12_CLEAR_FLAG_DEPTH : 0) |
			(clearStencil ? D3D12_CLEAR_FLAG_STENCIL : 0);
		mContext->GetCommandList()->ClearDepthStencilView(descriptor.Get(), D3D12_CLEAR_FLAGS(flag), depth, stencil, 0, nullptr);
	}


	void D3D12GraphicsRecorder::AddCopyOperation(GI::IGraphicMemoryResource* dest, GI::IGraphicMemoryResource* src)
	{
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();

		auto destId = (dest)->GetResourceId();
		auto srcId = (src)->GetResourceId();
		mContext->CopyResource(resourceManager->GetResource(destId), resourceManager->GetResource(srcId));
	}


	void D3D12GraphicsRecorder::AddCopyBufferToTexture(GI::IGraphicMemoryResource* destTexture, const GI::TextureSubresourceDesc& subresourceDesc, GI::IGraphicMemoryResource* srcBuffer)
	{
		Assert(destTexture->GetDimension() != GI::ResourceDimension::BUFFER);
		Assert(srcBuffer->GetDimension() == GI::ResourceDimension::BUFFER);

		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();
		auto destRes = resourceManager->GetResource(destTexture->GetResourceId());
		auto srcRes = resourceManager->GetResource(srcBuffer->GetResourceId());

		destRes->Transition(mContext, D3D12_RESOURCE_STATE_COPY_DEST);
		if (srcRes->GetHeapType() == GI::HeapType::DEFAULT) { srcRes->Transition(mContext, D3D12_RESOURCE_STATE_COPY_SOURCE); }

		auto subresIndex = subresourceDesc.GetSubresourceIndex(destTexture->GetDimension(), destTexture->GetSize().z(), destTexture->GetMipLevelCount(), 1);

		CD3DX12_TEXTURE_COPY_LOCATION CopyDest(destRes->GetD3D12Resource(), subresIndex);

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> bufferLayouts(subresIndex + 1);
		const auto& descDesc = destRes->GetD3D12Resource()->GetDesc();
		mContext->GetDevice()->GetDevice()->GetCopyableFootprints(&descDesc, 0, subresIndex + 1, 0, bufferLayouts.data(), nullptr, nullptr, nullptr);

		CD3DX12_TEXTURE_COPY_LOCATION CopySrc(
			srcRes->GetD3D12Resource(),
			bufferLayouts[subresIndex]);

		mContext->GetCommandList()->CopyTextureRegion(&CopyDest, 0, 0, 0, &CopySrc, nullptr);
	}

	void D3D12GraphicsRecorder::AddGraphicsPass(const GI::GraphicsPass& pass)
	{
		Assert(pass.IsReadyForExecute());
		
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();

		// transitions
		for (const auto& [_, srv] : pass.mSrvParams)
		{
			auto res = resourceManager->GetResource(srv.GetResourceId());
			res->Transition(mContext, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		for (auto i = 0; i < pass.mRtvCount; ++i)
		{
			auto res = resourceManager->GetResource(pass.mRtvs[i].GetResourceId());
			res->Transition(mContext, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}

		if (pass.mHasDsv)
		{
			auto res = resourceManager->GetResource(pass.mDsv.GetResourceId());
			res->Transition(mContext, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		}

		// root signatures
		D3D12PipelineStateLibrary* psoLib = mContext->GetDevice()->GetPipelineStateLib();
		auto rootSignature = psoLib->CreateRootSignature("res/RootSignature/RootSignature.hlsl", "GraphicsRS");

		auto pso = std::make_unique< GraphicsPipelineState>();
		pso->SetRootSignature(rootSignature);

		// shader
		ShaderPiece* vs = mContext->GetDevice()->GetShaderLib()->CreateVs(pass.mVsFile.c_str(), pass.mShaderMacros);
		ShaderPiece* ps = mContext->GetDevice()->GetShaderLib()->CreatePs(pass.mPsFile.c_str(), pass.mShaderMacros);

		pso->SetVertexShader(CD3DX12_SHADER_BYTECODE(vs->GetShader()));
		pso->SetPixelShader(CD3DX12_SHADER_BYTECODE(ps->GetShader()));

		const auto& inputLayout = vs->GetInputLayout();
		D3D12_BLEND_DESC blendDesc;
		{
			blendDesc.AlphaToCoverageEnable = pass.mBlendDesc.GetAlphaToCoverageEnable();
			blendDesc.IndependentBlendEnable = pass.mBlendDesc.GetIndependentBlendEnable();
			for (auto i = 0; i < sizeof(blendDesc.RenderTarget) / sizeof(blendDesc.RenderTarget[0]); ++i)
			{
				blendDesc.RenderTarget[i].BlendEnable = pass.mBlendDesc.RtBlendDesc[i].GetBlendEnable();
				blendDesc.RenderTarget[i].LogicOpEnable = pass.mBlendDesc.RtBlendDesc[i].GetLogicOpEnable();
				blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND(pass.mBlendDesc.RtBlendDesc[i].GetSrcBlend());
				blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND(pass.mBlendDesc.RtBlendDesc[i].GetDestBlend());
				blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP(pass.mBlendDesc.RtBlendDesc[i].GetBlendOp());
				blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND(pass.mBlendDesc.RtBlendDesc[i].GetSrcBlendAlpha());
				blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND(pass.mBlendDesc.RtBlendDesc[i].GetDestBlendAlpha());
				blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP(pass.mBlendDesc.RtBlendDesc[i].GetBlendOpAlpha());
				blendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP(pass.mBlendDesc.RtBlendDesc[i].GetLogicOp());
				blendDesc.RenderTarget[i].RenderTargetWriteMask = pass.mBlendDesc.RtBlendDesc[i].GetRenderTargetWriteMask();
			}
		}
		pso->SetBlendState(blendDesc);
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
		{
			depthStencilDesc.DepthEnable = pass.mDepthStencilDesc.GetDepthEnable();
			depthStencilDesc.DepthWriteMask = pass.mDepthStencilDesc.GetDepthWriteAllRatherThanZero() ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC(pass.mDepthStencilDesc.GetDepthFunc());
			depthStencilDesc.StencilEnable = pass.mDepthStencilDesc.GetStencilEnable();
			depthStencilDesc.StencilReadMask = pass.mDepthStencilDesc.GetStencilReadMask();
			depthStencilDesc.StencilWriteMask = pass.mDepthStencilDesc.GetStencilWriteMask();
			depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP(pass.mDepthStencilDesc.FrontFace.GetStencilFailOp());
			depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP(pass.mDepthStencilDesc.FrontFace.GetStencilDepthFailOp());
			depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP(pass.mDepthStencilDesc.FrontFace.GetStencilPassOp());
			depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC(pass.mDepthStencilDesc.FrontFace.GetStencilFunc());
			depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP(pass.mDepthStencilDesc.BackFace.GetStencilFailOp());
			depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP(pass.mDepthStencilDesc.BackFace.GetStencilDepthFailOp());
			depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP(pass.mDepthStencilDesc.BackFace.GetStencilPassOp());
			depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC(pass.mDepthStencilDesc.BackFace.GetStencilFunc());
		}
		pso->SetDepthStencilState(depthStencilDesc);
		D3D12_RASTERIZER_DESC rastDesc;
		{
			rastDesc.FillMode = pass.mRasterizerDesc.GetFillSolidRatherThanWireframe() ? D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;
			rastDesc.CullMode = D3D12_CULL_MODE(pass.mRasterizerDesc.GetCullMode());
			rastDesc.FrontCounterClockwise = pass.mRasterizerDesc.GetFrontCounterClockwise();
			rastDesc.DepthBias = pass.mRasterizerDesc.GetDepthBias();
			rastDesc.DepthBiasClamp = pass.mRasterizerDesc.GetDepthBiasClamp();
			rastDesc.SlopeScaledDepthBias = pass.mRasterizerDesc.GetSlopeScaledDepthBias();
			rastDesc.DepthClipEnable = pass.mRasterizerDesc.GetDepthClipEnable();
			rastDesc.MultisampleEnable = pass.mRasterizerDesc.GetMultisampleEnable();
			rastDesc.AntialiasedLineEnable = pass.mRasterizerDesc.GetAntialiasedLineEnable();
			rastDesc.ForcedSampleCount = pass.mRasterizerDesc.GetForcedSampleCount();
			rastDesc.ConservativeRaster = pass.mRasterizerDesc.GetConservativeRaster() ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}
		pso->SetRasterizerState(rastDesc);

		pso->SetInputLayout((UINT)inputLayout.size(), inputLayout.data());

		// rts
		pso->SetRtCount(pass.mRtvCount);
		for (const auto& [i, rtv] : enumerate(pass.mRtvs | take(pass.mRtvCount)))
		{
			pso->SetRtvFormat(i, D3D12Utils::ToDxgiFormat(rtv.GetUsage().GetFormat()));
		}
		pso->SetDsvFormat(pass.mHasDsv ? D3D12Utils::ToDxgiFormat(pass.mDsv.GetUsage().GetFormat()) : DXGI_FORMAT_UNKNOWN);

		pso->Finalize(mContext->GetDevice()->GetPipelineStateLib());

		// resource bindings
		ID3D12GraphicsCommandList* commandList = mContext->GetCommandList();

		commandList->SetGraphicsRootSignature(rootSignature);
		commandList->SetPipelineState(pso->Get());

		std::set<ID3D12DescriptorHeap*> heaps;
		std::map<i32, CD3DX12_GPU_DESCRIPTOR_HANDLE> gpuBaseAddrs;

		// srvs
		{
			RuntimeDescriptorHeap* srvHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			std::map<std::string, InputSrvParam> srvBindings = vs->GetSrvBindings();
			auto psSrvBindings = ps->GetSrvBindings();
			srvBindings.merge(psSrvBindings);

			const auto& bindPoints = srvBindings | views::transform([](const auto& p) { return p.second.mBindPoint; });
			i32 maxSrvIndex = bindPoints.size() ? *max_element(bindPoints) : 0;

			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srvHandles(maxSrvIndex + 1, mContext->GetDevice()->GetNullSrvUavCbvCpuDesc().Get());
			for (const auto& p : srvBindings)
			{
				const std::string& srvName = p.first;
				const InputSrvParam& srvParam = p.second;

				auto it = pass.mSrvParams.find(srvName);
				if (it != pass.mSrvParams.end())
				{
					const auto& descriptor = resourceManager->CreateSrvDescriptor(it->second.GetResourceId(), it->second.GetUsage());
					srvHandles[srvParam.mBindPoint] = descriptor.Get();
				}
			}
			const auto& gpuDescBaseAddr = srvHeap->Push(static_cast<i32>(srvHandles.size()), srvHandles.data());

			heaps.insert(srvHeap->GetCurrentDescriptorHeap());
			gpuBaseAddrs[0] = gpuDescBaseAddr;
		}

		{
			RuntimeDescriptorHeap* samplerHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

			std::map<std::string, DescriptorPtr> samplers;
			for (const auto& [name, sampler] : pass.mSamplerParams)
			{
				samplers[name] = resourceManager->CreateSampler(sampler);
			}
			const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& samplerHandles = BindSrvUavParams(mContext, ps->GetSamplerBindings(), samplers, mContext->GetDevice()->GetNullSamplerCpuDesc());

			const auto& gpuDescBase = samplerHeap->Push(static_cast<i32>(samplerHandles.size()), samplerHandles.data());

			heaps.insert(samplerHeap->GetCurrentDescriptorHeap());
			gpuBaseAddrs[2] = gpuDescBase;
		}

		std::vector<ID3D12DescriptorHeap*> heapArr(heaps.begin(), heaps.end());
		commandList->SetDescriptorHeaps(heapArr.size(), heapArr.data());
		for (const auto& [rsSlot, gpuBaseAddr] : gpuBaseAddrs)
		{
			commandList->SetGraphicsRootDescriptorTable(rsSlot, gpuBaseAddr);
		}

		// cbs
		std::vector<b8> cbufData;
		BindConstBufferParams(cbufData, pass.mCbParams, vs);
		BindConstBufferParams(cbufData, pass.mCbParams, ps);
		const D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = mContext->GetConstantBuffer()->Push(cbufData.data(), cbufData.size());
		mContext->GetCommandList()->SetGraphicsRootConstantBufferView(1, gpuAddr);

		Assert(pass.mViewPort.GetMinDepth() < pass.mViewPort.GetMaxDepth());
		auto viewport = D3D12_VIEWPORT{
			pass.mViewPort.GetTopLeftX(), pass.mViewPort.GetTopLeftY(),
			pass.mViewPort.GetWidth(), pass.mViewPort.GetHeight(),
			pass.mViewPort.GetMinDepth(), pass.mViewPort.GetMaxDepth()
		};
		commandList->RSSetViewports(1, &viewport);
		auto rect = D3D12_RECT{
			pass.mScissorRect.left,
			pass.mScissorRect.top,
			pass.mScissorRect.right,
			pass.mScissorRect.bottom };
		commandList->RSSetScissorRects(1, &rect);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandles[8] = {};
		for (auto i = 0; i < pass.mRtvCount; ++i)
		{
			rtvHandles[i] = resourceManager->CreateRtvDescriptor(pass.mRtvs[i].GetResourceId(), pass.mRtvs[i].GetUsage()).Get();
		}

		if (pass.mHasDsv)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsHandle = resourceManager->CreateDsvDescriptor(pass.mDsv.GetResourceId(), pass.mDsv.GetUsage()).Get();
			commandList->OMSetRenderTargets(pass.mRtvCount, rtvHandles, false, &dsHandle);
		}
		else
		{
			commandList->OMSetRenderTargets(pass.mRtvCount, rtvHandles, false, nullptr);
		}
		commandList->OMSetStencilRef(pass.mStencilRef);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		std::vector<D3D12_VERTEX_BUFFER_VIEW> vbvs(pass.mVbvs.size(), D3D12_VERTEX_BUFFER_VIEW{});
		for (auto i = 0; i < vbvs.size(); ++i)
		{
			vbvs[i].BufferLocation = resourceManager->GetResource(pass.mVbvs[i].GetResourceId())->GetD3D12Resource()->GetGPUVirtualAddress();
			vbvs[i].SizeInBytes = pass.mVbvs[i].GetUsage().GetSizeInBytes();
			vbvs[i].StrideInBytes = pass.mVbvs[i].GetUsage().GetStrideInBytes();
		}
		commandList->IASetVertexBuffers(0, static_cast<u32>(vbvs.size()), vbvs.data());

		D3D12_INDEX_BUFFER_VIEW ibv;
		{
			ibv.BufferLocation = resourceManager->GetResource(pass.mIbv.GetResourceId())->GetD3D12Resource()->GetGPUVirtualAddress();
			ibv.SizeInBytes = pass.mIbv.GetUsage().GetSizeInBytes();
			ibv.Format = D3D12Utils::ToDxgiFormat(pass.mIbv.GetUsage().GetFormat());
		}
		commandList->IASetIndexBuffer(&ibv);

		commandList->DrawIndexedInstanced(pass.mIndexCount, pass.mInstanceCount, pass.mIndexStartLocation, pass.mVertexStartLocation, 0);
	}

	void D3D12GraphicsRecorder::AddComputePass(const GI::ComputePass& pass)
	{
		Assert(pass.IsReadyForExecute());

		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();

		// transitions
		for (const auto& [_, srv] : pass.mSrvParams)
		{
			auto res = resourceManager->GetResource(srv.GetResourceId());
			res->Transition(mContext, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		for (const auto& [_, uav] : pass.mUavParams)
		{
			auto res = resourceManager->GetResource(uav.GetResourceId());
			res->Transition(mContext, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		}

		// root signatures
		D3D12PipelineStateLibrary* psoLib = mContext->GetDevice()->GetPipelineStateLib();
		auto rootSignature = psoLib->CreateRootSignature("res/RootSignature/RootSignature.hlsl", "ComputeRS");

		auto mPso = std::make_unique< ComputePipelineState>();
		mPso->SetRootSignature(rootSignature);

		// shader
		ShaderPiece* cs = mContext->GetDevice()->GetShaderLib()->CreateCs(pass.mCsFile.c_str(), pass.mShaderMacros);
		mPso->SetComputeShader(CD3DX12_SHADER_BYTECODE(cs->GetShader()));

		mPso->Finalize(mContext->GetDevice()->GetPipelineStateLib());

		ID3D12GraphicsCommandList* commandList = mContext->GetCommandList();

		commandList->SetComputeRootSignature(rootSignature);
		commandList->SetPipelineState(mPso->Get());

		std::set<ID3D12DescriptorHeap*> heaps;
		std::map<i32, CD3DX12_GPU_DESCRIPTOR_HANDLE> gpuBaseAddrs;


		RuntimeDescriptorHeap* srvUavHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		{
			// srv
			std::map<std::string, DescriptorPtr> srvs;
			for (const auto& [name, srv] : pass.mSrvParams)
			{
				srvs[name] = resourceManager->CreateSrvDescriptor(srv.GetResourceId(), srv.GetUsage());
			}
			const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srvHandles = BindSrvUavParams(mContext, cs->GetSrvBindings(), srvs, mContext->GetDevice()->GetNullSrvUavCbvCpuDesc());
			
			// uav
			std::map<std::string, DescriptorPtr> uavs;
			for (const auto& [name, uav] : pass.mUavParams)
			{
				uavs[name] = resourceManager->CreateUavDescriptor(uav.GetResourceId(), uav.GetUsage());
			}
			const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& uavHandles = BindSrvUavParams(mContext, cs->GetUavBindings(), uavs, mContext->GetDevice()->GetNullSrvUavCbvCpuDesc());

			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;
			handles.insert(handles.end(), srvHandles.begin(), srvHandles.end());
			handles.insert(handles.end(), uavHandles.begin(), uavHandles.end());

			const CD3DX12_GPU_DESCRIPTOR_HANDLE& gpuDescBase = srvUavHeap->Push(static_cast<i32>(handles.size()), handles.data());
			const u32 handleSize = srvUavHeap->GetDescHandleSize();

			gpuBaseAddrs[0] = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuDescBase, 0, handleSize);
			gpuBaseAddrs[1] = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuDescBase, srvHandles.size(), handleSize);

			heaps.insert(srvUavHeap->GetCurrentDescriptorHeap());
		}

		RuntimeDescriptorHeap* samplerHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		{
			std::map<std::string, DescriptorPtr> samplers;
			for (const auto& [name, sampler] : pass.mSamplerParams)
			{
				samplers[name] = resourceManager->CreateSampler(sampler);
			}
			const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& samplerHandles = BindSrvUavParams(mContext, cs->GetSamplerBindings(), samplers, mContext->GetDevice()->GetNullSamplerCpuDesc());

			const auto& gpuDescBase = samplerHeap->Push(static_cast<i32>(samplerHandles.size()), samplerHandles.data());

			heaps.insert(samplerHeap->GetCurrentDescriptorHeap());
			gpuBaseAddrs[3] = gpuDescBase;
		}

		std::vector<ID3D12DescriptorHeap*> heapArr(heaps.begin(), heaps.end());
		commandList->SetDescriptorHeaps(heapArr.size(), heapArr.data());
		for (const auto& [rsSlot, gpuBaseAddr] : gpuBaseAddrs)
		{
			commandList->SetComputeRootDescriptorTable(rsSlot, gpuBaseAddr);
		}

		std::vector<b8> cbufData;
		BindConstBufferParams(cbufData, pass.mCbParams, cs);
		const D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = mContext->GetConstantBuffer()->Push(cbufData.data(), cbufData.size());
		mContext->GetCommandList()->SetComputeRootConstantBufferView(2, gpuAddr);

		commandList->Dispatch(pass.mThreadGroupCounts[0], pass.mThreadGroupCounts[1], pass.mThreadGroupCounts[2]);
	}

	void D3D12GraphicsRecorder::AddPreparePresent(GI::IGraphicMemoryResource* res)
	{
		auto resId = res->GetResourceId();
		auto devieRes = mContext->GetDevice()->GetResourceManager()->GetResource(resId);
		devieRes->Transition(mContext, D3D12_RESOURCE_STATE_PRESENT);
	}

	void D3D12GraphicsRecorder::AddBeginEvent(const char* mark)
	{
#if defined(_PIX_H_) || defined(_PIX3_H_)
		PIXBeginEvent(mContext->GetCommandList(), 0, mark);
#endif
	}


	void D3D12GraphicsRecorder::AddEndEvent()
	{
#if defined(_PIX_H_) || defined(_PIX3_H_)
				PIXEndEvent(mContext->GetCommandList());
#endif
	}

	// TODO investigate this https://stackoverflow.com/a/20669290/2131563
	//int func()
	//{
	//	auto ptr = std::unique_ptr<int>(new int{ 1 });

	//	auto logInt = [ptr = std::move(ptr)](){ *ptr; };
	//	
	//	std::vector<std::function<void()>> loggers;
	//	loggers.push_back(std::move(logInt));
	//}

	void D3D12GraphicsRecorder::Finalize(bool dropAllCommands)
	{
		mContext->Finalize();
	}
}

GI::IGraphicsInfra* CreateGraphicsInfra()
{
	return new D3D12Backend::D3D12GraphicsInfra();
}