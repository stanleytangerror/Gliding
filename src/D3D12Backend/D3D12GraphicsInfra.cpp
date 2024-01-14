#include "D3D12BackendPch.h"
#include "D3D12GraphicsInfra.h"
#include "D3D12Resource.h"
#include "D3D12SwapChain.h"
#include "Common/GraphicsInfrastructure.h"

namespace D3D12Backend
{
	D3D12GraphicsInfra::D3D12GraphicsInfra()
	{
		mDevice = new D3D12Device;
	}

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
		return mDevice->GetSwapChainBuffers(PresentPortType(windowId))->GetBuffer()->GetRtv();
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
		mCurrentRecorder = new D3D12GraphicsRecorder(context);
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

	namespace
	{
		i32 BindConstBufferParams(std::vector<b8>& cbuf, const std::map<std::string, std::vector<b8>>& cbArgs, ShaderPiece* shader)
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
						Assert(varDesc.mSize == varArg.size());
						memcpy_s(cbuf.data() + varDesc.mStartOffset, varDesc.mSize, varArg.data(), varArg.size());
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

	void D3D12GraphicsRecorder::AddClearOperation(const GI::RtvDesc& rtv, const Vec4f& value)
	{
		const auto& descriptor = mContext->GetDevice()->GetResourceManager()->CreateRtvDescriptor(rtv);
		float rgba[4] = { value.x(), value.y(), value.z(), value.w() };
		mContext->GetCommandList()->ClearRenderTargetView(descriptor.Get(), rgba, 0, nullptr);
	}

	void D3D12GraphicsRecorder::AddClearOperation(const GI::DsvDesc& dsv, bool clearDepth, float depth, bool clearStencil, u32 stencil)
	{
		const auto& descriptor = mContext->GetDevice()->GetResourceManager()->CreateDsvDescriptor(dsv);
		auto flag =
			(clearDepth ? D3D12_CLEAR_FLAG_DEPTH : 0) |
			(clearStencil ? D3D12_CLEAR_FLAG_STENCIL : 0);
		mContext->GetCommandList()->ClearDepthStencilView(descriptor.Get(), D3D12_CLEAR_FLAGS(flag), depth, stencil, 0, nullptr);
	}


	void D3D12GraphicsRecorder::AddCopyOperation(GI::IGraphicMemoryResource* dest, GI::IGraphicMemoryResource* src)
	{
		mContext->CopyResource(
			reinterpret_cast<CommitedResource*>(dest),
			reinterpret_cast<CommitedResource*>(src));
	}

	void D3D12GraphicsRecorder::AddGraphicsPass(const GI::GraphicsPass& pass)
	{
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();

		// transitions
		for (const auto& [_, srv] : pass.mSrvParams)
		{
			auto res = reinterpret_cast<CommitedResource*>(srv.GetResource());
			res->Transition(mContext, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		for (const auto& rtv : pass.mRtvs)
		{
			if (rtv.GetResource())
			{
				auto res = reinterpret_cast<CommitedResource*>(rtv.GetResource());
				res->Transition(mContext, D3D12_RESOURCE_STATE_RENDER_TARGET);
			}
		}

		if (pass.mDsv.GetResource())
		{
			auto res = reinterpret_cast<CommitedResource*>(pass.mDsv.GetResource());
			res->Transition(mContext, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		}

		// root signatures
		D3D12PipelineStateLibrary* psoLib = mContext->GetDevice()->GetPipelineStateLib();
		auto rootSignature = psoLib->CreateRootSignature(pass.mRootSignatureDesc.mFile.c_str(), pass.mRootSignatureDesc.mEntry);

		auto mPso = std::make_unique< GraphicsPipelineState>();
		mPso->SetRootSignature(rootSignature);

		// shader
		ShaderPiece* vs = mContext->GetDevice()->GetShaderLib()->CreateVs(pass.mVsFile.c_str(), pass.mShaderMacros);
		ShaderPiece* ps = mContext->GetDevice()->GetShaderLib()->CreatePs(pass.mPsFile.c_str(), pass.mShaderMacros);

		mPso->SetVertexShader(CD3DX12_SHADER_BYTECODE(vs->GetShader()));
		mPso->SetPixelShader(CD3DX12_SHADER_BYTECODE(ps->GetShader()));

		const auto& inputLayout = vs->GetInputLayout();
		mPso->SetInputLayout((UINT)inputLayout.size(), inputLayout.data());

		// rts
		auto& desc = mPso->Descriptor();
		desc.NumRenderTargets = static_cast<u32>(pass.mRtvs.size());
		for (auto i = 0; i < pass.mRtvs.size(); ++i)
		{
			const auto& rtv = pass.mRtvs[i];
			if (rtv.GetResource()) 
			{
				desc.NumRenderTargets = i;
				break;
			}

			desc.RTVFormats[i] = D3D12Utils::ToDxgiFormat(rtv.GetFormat());
		}

		if (pass.mDsv.GetResource())
		{
			desc.DSVFormat = D3D12Utils::ToDxgiFormat(pass.mDsv.GetFormat());
		}

		mPso->Finalize(mContext->GetDevice()->GetPipelineStateLib());

		// resource bindings
		ID3D12GraphicsCommandList* commandList = mContext->GetCommandList();

		commandList->SetGraphicsRootSignature(rootSignature);
		commandList->SetPipelineState(mPso->Get());

		std::set<ID3D12DescriptorHeap*> heaps;
		std::map<i32, CD3DX12_GPU_DESCRIPTOR_HANDLE> gpuBaseAddrs;

		// srvs
		{
			RuntimeDescriptorHeap* srvHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			std::map<std::string, InputSrvParam> srvBindings;
			{
				for (const auto& p : vs->GetSrvBindings())
				{
					if (srvBindings.find(p.first) == srvBindings.end())
					{
						srvBindings[p.first] = p.second;
					}
				}
				for (const auto& p : ps->GetSrvBindings())
				{
					if (srvBindings.find(p.first) == srvBindings.end())
					{
						srvBindings[p.first] = p.second;
					}
				}
			}

			int maxSrvIndex = 0;
			for (const auto& p : srvBindings)
			{
				const InputSrvParam& srvParam = p.second;
				maxSrvIndex = std::max<int>(maxSrvIndex, srvParam.mBindPoint);
			}

			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srvHandles(maxSrvIndex + 1, mContext->GetDevice()->GetNullSrvUavCbvCpuDesc().Get());
			for (const auto& p : srvBindings)
			{
				const std::string& srvName = p.first;
				const InputSrvParam& srvParam = p.second;

				auto it = pass.mSrvParams.find(srvName);
				if (it != pass.mSrvParams.end())
				{
					const auto& srv = it->second;
					srvHandles[srvParam.mBindPoint] = resourceManager->CreateSrvDescriptor(srv).Get();
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
		u32 rtvCount = 0;
		for (; rtvCount < pass.mRtvs.size(); ++rtvCount)
		{
			if (!pass.mRtvs[rtvCount].GetResource())
			{
				break;
			}
			rtvHandles[rtvCount] = resourceManager->CreateRtvDescriptor(pass.mRtvs[rtvCount]).Get();
		}

		if (pass.mDsv.GetResource())
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsHandle = resourceManager->CreateDsvDescriptor(pass.mDsv).Get();
			commandList->OMSetRenderTargets(rtvCount, rtvHandles, false, &dsHandle);
		}
		else
		{
			commandList->OMSetRenderTargets(rtvCount, rtvHandles, false, nullptr);
		}
		commandList->OMSetStencilRef(pass.mStencilRef);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		std::vector<D3D12_VERTEX_BUFFER_VIEW> vbvs(pass.mVbvs.size(), D3D12_VERTEX_BUFFER_VIEW{});
		for (auto i = 0; i < vbvs.size(); ++i)
		{
			vbvs[i].BufferLocation = reinterpret_cast<const CommitedResource*>(pass.mVbvs[i].GetResource())->GetD3D12Resource()->GetGPUVirtualAddress();
			vbvs[i].SizeInBytes = pass.mVbvs[i].GetSizeInBytes();
			vbvs[i].StrideInBytes = pass.mVbvs[i].GetStrideInBytes();
		}
		commandList->IASetVertexBuffers(0, static_cast<u32>(vbvs.size()), vbvs.data());

		D3D12_INDEX_BUFFER_VIEW ibv;
		{
			ibv.BufferLocation = reinterpret_cast<const CommitedResource*>(pass.mIbv.GetResource())->GetD3D12Resource()->GetGPUVirtualAddress();
			ibv.SizeInBytes = pass.mIbv.GetSizeInBytes();
			ibv.Format = D3D12Utils::ToDxgiFormat(pass.mIbv.GetFormat());
		}
		commandList->IASetIndexBuffer(&ibv);

		commandList->DrawIndexedInstanced(pass.mIndexCount, pass.mInstanceCount, pass.mIndexStartLocation, pass.mVertexStartLocation, 0);
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