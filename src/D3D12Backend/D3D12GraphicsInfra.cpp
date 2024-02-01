#include "D3D12BackendPch.h"
#include "D3D12GraphicsInfra.h"
#include "D3D12Resource.h"
#include "D3D12SwapChain.h"
#include "Common/GraphicsInfrastructure.h"
#include "../packages/WinPixEventRuntime.1.0.231030001/Include/WinPixEventRuntime/pix3.h"

namespace D3D12Backend
{
	D3D12GraphicsInfra::D3D12GraphicsInfra()
	{
		mDevice = new D3D12Device;
	}

	std::unique_ptr<GI::IGraphicMemoryResource> D3D12GraphicsInfra::CreateMemoryResource(const GI::MemoryResourceDesc& desc)
	{
		return std::move(mDevice->GetResourceManager()->CreateResource(desc));
	}

	std::unique_ptr<GI::IGraphicMemoryResource> D3D12GraphicsInfra::CreateMemoryResource(const GI::IImage& image)
	{
		const auto& dxScratchImage = *(reinterpret_cast<const D3D12Utils::WindowsImage*>(&image));
		return D3D12Utils::CreateResourceFromImage(mCurrentRecorder->GetContext(), dxScratchImage);
	}

	void D3D12GraphicsInfra::CopyToUploadBufferResource(GI::IGraphicMemoryResource* resource, const std::vector<b8>& data)
	{
		Assert(resource->GetDimension() == GI::ResourceDimension::BUFFER);
		Assert(resource->GetSize().x() >= data.size());

		auto dx12Res = mDevice->GetResourceManager()->GetResource(resource->GetResourceId());
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
		mDevice->GetResourceManager()->CreateSwapChain(windowId, HWND(windowInfo.mNativeHandle), windowInfo.mSize, windowInfo.mFrameCount);
	}


	void D3D12GraphicsInfra::ResizeWindow(u8 windowId, const Vec2u& windowSize)
	{
		auto swapChain = mDevice->GetResourceManager()->GetSwapChain(windowId);

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

	GI::IGraphicMemoryResource* D3D12GraphicsInfra::GetWindowBackBuffer(u8 windowId)
	{
		return mDevice->GetResourceManager()->GetSwapChain(windowId)->GetBuffer();
	}

	void D3D12GraphicsInfra::StartFrame()
	{
		mDevice->StartFrame();
		StartRecording();
	}

	void D3D12GraphicsInfra::EndFrame()
	{
		mCurrentRecorder->AddPreparePresent(GetWindowBackBuffer(u8(PresentPortType::MainPort)));
		mCurrentRecorder->AddPreparePresent(GetWindowBackBuffer(u8(PresentPortType::DebugPort)));

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

#define DEFERRED_EXECUTE 1
	void D3D12GraphicsRecorder::AddClearOperation(const GI::RtvUsage& rtv, const Vec4f& value)
	{
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();
		auto resId = rtv.GetResourceId();

#if DEFERRED_EXECUTE
		mCommands.push([this, resourceManager, resId, rtv, value]()
			{
				resourceManager->GetResource(resId)->Transition(mContext, D3D12_RESOURCE_STATE_RENDER_TARGET);

				const auto& descriptor = mContext->GetDevice()->GetResourceManager()->CreateRtvDescriptor(resId, rtv);
				float rgba[4] = { value.x(), value.y(), value.z(), value.w() };
				mContext->GetCommandList()->ClearRenderTargetView(descriptor.Get(), rgba, 0, nullptr);
			});
#else
				resourceManager->GetResource(resId)->Transition(mContext, D3D12_RESOURCE_STATE_RENDER_TARGET);

				const auto& descriptor = mContext->GetDevice()->GetResourceManager()->CreateRtvDescriptor(rtv.GetResourceId(), rtv);
				float rgba[4] = { value.x(), value.y(), value.z(), value.w() };
				mContext->GetCommandList()->ClearRenderTargetView(descriptor.Get(), rgba, 0, nullptr);
#endif
	}

	void D3D12GraphicsRecorder::AddClearOperation(const GI::DsvUsage& dsv, bool clearDepth, float depth, bool clearStencil, u32 stencil)
	{
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();
		auto resId = dsv.GetResourceId();

#if DEFERRED_EXECUTE
		mCommands.push([this, resourceManager, resId, dsv, clearDepth, depth, clearStencil, stencil]()
			{
				resourceManager->GetResource(resId)->Transition(mContext, D3D12_RESOURCE_STATE_DEPTH_WRITE);

				const auto& descriptor = mContext->GetDevice()->GetResourceManager()->CreateDsvDescriptor(resId, dsv);
				auto flag =
					(clearDepth ? D3D12_CLEAR_FLAG_DEPTH : 0) |
					(clearStencil ? D3D12_CLEAR_FLAG_STENCIL : 0);
				mContext->GetCommandList()->ClearDepthStencilView(descriptor.Get(), D3D12_CLEAR_FLAGS(flag), depth, stencil, 0, nullptr);
			});
#else
		resourceManager->GetResource(resId)->Transition(mContext, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		const auto& descriptor = mContext->GetDevice()->GetResourceManager()->CreateDsvDescriptor(dsv.GetResourceId(), dsv);
		auto flag =
			(clearDepth ? D3D12_CLEAR_FLAG_DEPTH : 0) |
			(clearStencil ? D3D12_CLEAR_FLAG_STENCIL : 0);
		mContext->GetCommandList()->ClearDepthStencilView(descriptor.Get(), D3D12_CLEAR_FLAGS(flag), depth, stencil, 0, nullptr);
#endif
	}


	void D3D12GraphicsRecorder::AddCopyOperation(GI::IGraphicMemoryResource* dest, GI::IGraphicMemoryResource* src)
	{
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();

		auto destId = (dest)->GetResourceId();
		auto srcId = (src)->GetResourceId();
#if DEFERRED_EXECUTE
		mCommands.push([this, resourceManager, destId, srcId]()
			{
				mContext->CopyResource(resourceManager->GetResource(destId), resourceManager->GetResource(srcId));
			});
#else
		mContext->CopyResource(resourceManager->GetResource(destId), resourceManager->GetResource(srcId));
#endif

	}

	void D3D12GraphicsRecorder::AddGraphicsPass(const GI::GraphicsPass& pass)
	{
		if (!pass.IsReadyForExecute()) { return; }

		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();

		D3D12Backend::GraphicsPass d3d12Pass;
		d3d12Pass.mRootSignatureDesc = pass.mRootSignatureDesc;
		d3d12Pass.mVsFile = pass.mVsFile;
		d3d12Pass.mPsFile = pass.mPsFile;
		d3d12Pass.mShaderMacros = pass.mShaderMacros;
		std::transform(pass.mRtvs.begin(), pass.mRtvs.begin() + pass.mRtvCount, d3d12Pass.mRtvs.begin(),
			[](const auto& rtv) { return RtvUsageImpl(rtv.GetResource()->GetResourceId(), rtv); });
		if (pass.mHasDsv)
		{
			d3d12Pass.mDsv = DsvUsageImpl(pass.mDsv.GetResource()->GetResourceId(), pass.mDsv);
		}
		d3d12Pass.mInputLayout = pass.mInputLayout;
		d3d12Pass.mRasterizerDesc = pass.mRasterizerDesc;
		d3d12Pass.mDepthStencilDesc = pass.mDepthStencilDesc;
		d3d12Pass.mBlendDesc = pass.mBlendDesc;
		d3d12Pass.mVbvs.resize(pass.mVbvs.size());
		std::transform(pass.mVbvs.begin(), pass.mVbvs.end(), d3d12Pass.mVbvs.begin(),
			[](const auto& vbv) { return VbvUsageImpl(vbv.GetResource()->GetResourceId(), vbv); });
		d3d12Pass.mIbv = IbvUsageImpl(pass.mIbv.GetResource()->GetResourceId(), pass.mIbv);
		d3d12Pass.mVertexStartLocation = pass.mVertexStartLocation;
		d3d12Pass.mIndexStartLocation = pass.mIndexStartLocation;
		d3d12Pass.mIndexCount = pass.mIndexCount;
		d3d12Pass.mInstanceCount = pass.mInstanceCount;
		d3d12Pass.mViewPort = pass.mViewPort;
		d3d12Pass.mScissorRect = pass.mScissorRect;
		d3d12Pass.mStencilRef = pass.mStencilRef;
		d3d12Pass.mCbParams = pass.mCbParams;
		d3d12Pass.mSamplerParams = pass.mSamplerParams;
		for (const auto& [name, srv] : pass.mSrvParams)
		{
			d3d12Pass.mSrvParams[name] = SrvUsageImpl(srv.GetResource()->GetResourceId(), srv);
		}

#if DEFERRED_EXECUTE
		mCommands.push([this, resourceManager, d3d12Pass]()
			{
#endif
				for (const auto& vbv : d3d12Pass.mVbvs)
				{
					if (!vbv.GetResourceId()) { return; }
				}

				std::map<std::string, DescriptorPtr> srvs;
				for (const auto& [name, srv] : d3d12Pass.mSrvParams)
				{
					if (!srv.GetResourceId()) { return; }
					srvs[name] = resourceManager->CreateSrvDescriptor(srv.GetResourceId(), srv);
				}

				std::vector<DescriptorPtr> rtvs;
				std::vector<DXGI_FORMAT> rtvFormats;
				auto numRtvs = std::count_if(d3d12Pass.mRtvs.begin(), d3d12Pass.mRtvs.end(), [](const auto& rtv) { return rtv.GetResourceId(); });
				if (numRtvs == 0 && !d3d12Pass.mDsv.GetResourceId()) { return; }
				for (auto i = 0; i < numRtvs; ++i)
				{
					rtvs.push_back(resourceManager->CreateRtvDescriptor(d3d12Pass.mRtvs[i].GetResourceId(), d3d12Pass.mRtvs[i]));
					rtvFormats.push_back(D3D12Utils::ToDxgiFormat(d3d12Pass.mRtvs[i].GetFormat()));
				}

				std::map<std::string, DescriptorPtr> samplers;
				for (const auto& [name, sampler] : d3d12Pass.mSamplerParams)
				{
					samplers[name] = resourceManager->CreateSampler(sampler);
				}


				// transitions
				for (const auto& [_, srv] : d3d12Pass.mSrvParams)
				{
					auto res = resourceManager->GetResource(srv.GetResourceId());
					res->Transition(mContext, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				}

				for (const auto& rtv : d3d12Pass.mRtvs)
				{
					if (rtv.GetResourceId())
					{
						auto res = resourceManager->GetResource(rtv.GetResourceId());
						res->Transition(mContext, D3D12_RESOURCE_STATE_RENDER_TARGET);
					}
				}

				if (d3d12Pass.mDsv.GetResourceId())
				{
					auto res = resourceManager->GetResource(d3d12Pass.mDsv.GetResourceId());
					res->Transition(mContext, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				}

				// root signatures
				D3D12PipelineStateLibrary* psoLib = mContext->GetDevice()->GetPipelineStateLib();
				auto rootSignature = psoLib->CreateRootSignature(d3d12Pass.mRootSignatureDesc.mFile.c_str(), d3d12Pass.mRootSignatureDesc.mEntry);

				auto pso = std::make_unique< GraphicsPipelineState>();
				pso->SetRootSignature(rootSignature);

				// shader
				ShaderPiece* vs = mContext->GetDevice()->GetShaderLib()->CreateVs(d3d12Pass.mVsFile.c_str(), d3d12Pass.mShaderMacros);
				ShaderPiece* ps = mContext->GetDevice()->GetShaderLib()->CreatePs(d3d12Pass.mPsFile.c_str(), d3d12Pass.mShaderMacros);

				pso->SetVertexShader(CD3DX12_SHADER_BYTECODE(vs->GetShader()));
				pso->SetPixelShader(CD3DX12_SHADER_BYTECODE(ps->GetShader()));

				const auto& inputLayout = vs->GetInputLayout();
				D3D12_BLEND_DESC blendDesc;
				{
					blendDesc.AlphaToCoverageEnable = d3d12Pass.mBlendDesc.GetAlphaToCoverageEnable();
					blendDesc.IndependentBlendEnable = d3d12Pass.mBlendDesc.GetIndependentBlendEnable();
					for (auto i = 0; i < sizeof(blendDesc.RenderTarget) / sizeof(blendDesc.RenderTarget[0]); ++i)
					{
						blendDesc.RenderTarget[i].BlendEnable = d3d12Pass.mBlendDesc.RtBlendDesc[i].GetBlendEnable();
						blendDesc.RenderTarget[i].LogicOpEnable = d3d12Pass.mBlendDesc.RtBlendDesc[i].GetLogicOpEnable();
						blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND(d3d12Pass.mBlendDesc.RtBlendDesc[i].GetSrcBlend());
						blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND(d3d12Pass.mBlendDesc.RtBlendDesc[i].GetDestBlend());
						blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP(d3d12Pass.mBlendDesc.RtBlendDesc[i].GetBlendOp());
						blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND(d3d12Pass.mBlendDesc.RtBlendDesc[i].GetSrcBlendAlpha());
						blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND(d3d12Pass.mBlendDesc.RtBlendDesc[i].GetDestBlendAlpha());
						blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP(d3d12Pass.mBlendDesc.RtBlendDesc[i].GetBlendOpAlpha());
						blendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP(d3d12Pass.mBlendDesc.RtBlendDesc[i].GetLogicOp());
						blendDesc.RenderTarget[i].RenderTargetWriteMask = d3d12Pass.mBlendDesc.RtBlendDesc[i].GetRenderTargetWriteMask();
					}
				}
				pso->SetBlendState(blendDesc);
				D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
				{
					depthStencilDesc.DepthEnable = d3d12Pass.mDepthStencilDesc.GetDepthEnable();
					depthStencilDesc.DepthWriteMask = d3d12Pass.mDepthStencilDesc.GetDepthWriteAllRatherThanZero() ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
					depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC(d3d12Pass.mDepthStencilDesc.GetDepthFunc());
					depthStencilDesc.StencilEnable = d3d12Pass.mDepthStencilDesc.GetStencilEnable();
					depthStencilDesc.StencilReadMask = d3d12Pass.mDepthStencilDesc.GetStencilReadMask();
					depthStencilDesc.StencilWriteMask = d3d12Pass.mDepthStencilDesc.GetStencilWriteMask();
					depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP(d3d12Pass.mDepthStencilDesc.FrontFace.GetStencilFailOp());
					depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP(d3d12Pass.mDepthStencilDesc.FrontFace.GetStencilDepthFailOp());
					depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP(d3d12Pass.mDepthStencilDesc.FrontFace.GetStencilPassOp());
					depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC(d3d12Pass.mDepthStencilDesc.FrontFace.GetStencilFunc());
					depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP(d3d12Pass.mDepthStencilDesc.BackFace.GetStencilFailOp());
					depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP(d3d12Pass.mDepthStencilDesc.BackFace.GetStencilDepthFailOp());
					depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP(d3d12Pass.mDepthStencilDesc.BackFace.GetStencilPassOp());
					depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC(d3d12Pass.mDepthStencilDesc.BackFace.GetStencilFunc());
				}
				pso->SetDepthStencilState(depthStencilDesc);
				D3D12_RASTERIZER_DESC rastDesc;
				{
					rastDesc.FillMode = d3d12Pass.mRasterizerDesc.GetFillSolidRatherThanWireframe() ? D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;
					rastDesc.CullMode = D3D12_CULL_MODE(d3d12Pass.mRasterizerDesc.GetCullMode());
					rastDesc.FrontCounterClockwise = d3d12Pass.mRasterizerDesc.GetFrontCounterClockwise();
					rastDesc.DepthBias = d3d12Pass.mRasterizerDesc.GetDepthBias();
					rastDesc.DepthBiasClamp = d3d12Pass.mRasterizerDesc.GetDepthBiasClamp();
					rastDesc.SlopeScaledDepthBias = d3d12Pass.mRasterizerDesc.GetSlopeScaledDepthBias();
					rastDesc.DepthClipEnable = d3d12Pass.mRasterizerDesc.GetDepthClipEnable();
					rastDesc.MultisampleEnable = d3d12Pass.mRasterizerDesc.GetMultisampleEnable();
					rastDesc.AntialiasedLineEnable = d3d12Pass.mRasterizerDesc.GetAntialiasedLineEnable();
					rastDesc.ForcedSampleCount = d3d12Pass.mRasterizerDesc.GetForcedSampleCount();
					rastDesc.ConservativeRaster = d3d12Pass.mRasterizerDesc.GetConservativeRaster() ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
				}
				pso->SetRasterizerState(rastDesc);

				pso->SetInputLayout((UINT)inputLayout.size(), inputLayout.data());

				// rts
				auto& desc = pso->Descriptor();
				desc.NumRenderTargets = numRtvs;
				for (auto i = 0; i < sizeof(desc.RTVFormats) / sizeof(desc.RTVFormats[0]); ++i)
				{
					desc.RTVFormats[i] = i < numRtvs ? rtvFormats[i] : DXGI_FORMAT_UNKNOWN;
				}

				if (d3d12Pass.mDsv.GetResourceId())
				{
					desc.DSVFormat = D3D12Utils::ToDxgiFormat(d3d12Pass.mDsv.GetFormat());
				}

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

						auto it = srvs.find(srvName);
						if (it != srvs.end())
						{
							srvHandles[srvParam.mBindPoint] = it->second.Get();
						}
					}
					const auto& gpuDescBaseAddr = srvHeap->Push(static_cast<i32>(srvHandles.size()), srvHandles.data());

					heaps.insert(srvHeap->GetCurrentDescriptorHeap());
					gpuBaseAddrs[0] = gpuDescBaseAddr;
				}

				{
					RuntimeDescriptorHeap* samplerHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

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
				BindConstBufferParams(cbufData, d3d12Pass.mCbParams, vs);
				BindConstBufferParams(cbufData, d3d12Pass.mCbParams, ps);
				const D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = mContext->GetConstantBuffer()->Push(cbufData.data(), cbufData.size());
				mContext->GetCommandList()->SetGraphicsRootConstantBufferView(1, gpuAddr);

				Assert(d3d12Pass.mViewPort.GetMinDepth() < d3d12Pass.mViewPort.GetMaxDepth());
				auto viewport = D3D12_VIEWPORT{
					d3d12Pass.mViewPort.GetTopLeftX(), d3d12Pass.mViewPort.GetTopLeftY(),
					d3d12Pass.mViewPort.GetWidth(), d3d12Pass.mViewPort.GetHeight(),
					d3d12Pass.mViewPort.GetMinDepth(), d3d12Pass.mViewPort.GetMaxDepth()
				};
				commandList->RSSetViewports(1, &viewport);
				auto rect = D3D12_RECT{
					d3d12Pass.mScissorRect.left,
					d3d12Pass.mScissorRect.top,
					d3d12Pass.mScissorRect.right,
					d3d12Pass.mScissorRect.bottom };
				commandList->RSSetScissorRects(1, &rect);

				CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandles[8] = {};
				u32 rtvCount = 0;
				for (; rtvCount < rtvs.size(); ++rtvCount)
				{
					if (!rtvs[rtvCount]) { break; }
					rtvHandles[rtvCount] = rtvs[rtvCount].Get();
				}

				if (d3d12Pass.mDsv.GetResourceId())
				{
					CD3DX12_CPU_DESCRIPTOR_HANDLE dsHandle = resourceManager->CreateDsvDescriptor(d3d12Pass.mDsv.GetResourceId(), d3d12Pass.mDsv).Get();
					commandList->OMSetRenderTargets(rtvCount, rtvHandles, false, &dsHandle);
				}
				else
				{
					commandList->OMSetRenderTargets(rtvCount, rtvHandles, false, nullptr);
				}
				commandList->OMSetStencilRef(d3d12Pass.mStencilRef);

				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				std::vector<D3D12_VERTEX_BUFFER_VIEW> vbvs(d3d12Pass.mVbvs.size(), D3D12_VERTEX_BUFFER_VIEW{});
				for (auto i = 0; i < vbvs.size(); ++i)
				{
					vbvs[i].BufferLocation = resourceManager->GetResource(d3d12Pass.mVbvs[i].GetResourceId())->GetD3D12Resource()->GetGPUVirtualAddress();
					vbvs[i].SizeInBytes = d3d12Pass.mVbvs[i].GetSizeInBytes();
					vbvs[i].StrideInBytes = d3d12Pass.mVbvs[i].GetStrideInBytes();
				}
				commandList->IASetVertexBuffers(0, static_cast<u32>(vbvs.size()), vbvs.data());

				D3D12_INDEX_BUFFER_VIEW ibv;
				{
					ibv.BufferLocation = resourceManager->GetResource(d3d12Pass.mIbv.GetResourceId())->GetD3D12Resource()->GetGPUVirtualAddress();
					ibv.SizeInBytes = d3d12Pass.mIbv.GetSizeInBytes();
					ibv.Format = D3D12Utils::ToDxgiFormat(d3d12Pass.mIbv.GetFormat());
				}
				commandList->IASetIndexBuffer(&ibv);

				commandList->DrawIndexedInstanced(d3d12Pass.mIndexCount, d3d12Pass.mInstanceCount, d3d12Pass.mIndexStartLocation, d3d12Pass.mVertexStartLocation, 0);
#if DEFERRED_EXECUTE
			});
#endif
	}

	void D3D12GraphicsRecorder::AddComputePass(const GI::ComputePass& pass)
	{
		if (!pass.IsReadyForExecute()) { return; }

		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();

		D3D12Backend::ComputePass d3d12Pass;

		d3d12Pass.mRootSignatureDesc = pass.mRootSignatureDesc;
		d3d12Pass.mCsFile = pass.mCsFile;
		d3d12Pass.mShaderMacros = pass.mShaderMacros;
		d3d12Pass.mCbParams = pass.mCbParams;
		for (const auto& [name, srv] : pass.mSrvParams)
		{
			d3d12Pass.mSrvParams[name] = SrvUsageImpl(srv.GetResource()->GetResourceId(), srv);
		}
		for (const auto& [name, uav] : pass.mUavParams)
		{
			d3d12Pass.mUavParams[name] = UavUsageImpl(uav.GetResource()->GetResourceId(), uav);
		}
		d3d12Pass.mSamplerParams = pass.mSamplerParams;
		d3d12Pass.mThreadGroupCounts = pass.mThreadGroupCounts;

#if DEFERRED_EXECUTE
		mCommands.push([this, resourceManager, d3d12Pass]()
			{
#endif
				std::map<std::string, DescriptorPtr> srvs;
				for (const auto& [name, srv] : d3d12Pass.mSrvParams)
				{
					if (!srv.GetResourceId()) { return; }
					srvs[name] = resourceManager->CreateSrvDescriptor(srv.GetResourceId(), srv);
				}

				std::map<std::string, DescriptorPtr> uavs;
				for (const auto& [name, uav] : d3d12Pass.mUavParams)
				{
					if (!uav.GetResourceId()) { return; }
					uavs[name] = resourceManager->CreateUavDescriptor(uav.GetResourceId(), uav);
				}

				std::map<std::string, DescriptorPtr> samplers;
				for (const auto& [name, sampler] : d3d12Pass.mSamplerParams)
				{
					samplers[name] = resourceManager->CreateSampler(sampler);
				}

				for (const auto& [_, srv] : d3d12Pass.mSrvParams)
				{
					auto res = resourceManager->GetResource(srv.GetResourceId());
					res->Transition(mContext, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				}

				for (const auto& [_, uav] : d3d12Pass.mUavParams)
				{
					auto res = resourceManager->GetResource(uav.GetResourceId());
					res->Transition(mContext, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				}

				// root signatures
				D3D12PipelineStateLibrary* psoLib = mContext->GetDevice()->GetPipelineStateLib();
				auto rootSignature = psoLib->CreateRootSignature(d3d12Pass.mRootSignatureDesc.mFile.c_str(), d3d12Pass.mRootSignatureDesc.mEntry);

				auto mPso = std::make_unique< ComputePipelineState>();
				mPso->SetRootSignature(rootSignature);

				// shader
				ShaderPiece* cs = mContext->GetDevice()->GetShaderLib()->CreateCs(d3d12Pass.mCsFile.c_str(), d3d12Pass.mShaderMacros);
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
					const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srvHandles = BindSrvUavParams(mContext, cs->GetSrvBindings(), srvs, mContext->GetDevice()->GetNullSrvUavCbvCpuDesc());
					// uav
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
				BindConstBufferParams(cbufData, d3d12Pass.mCbParams, cs);
				const D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = mContext->GetConstantBuffer()->Push(cbufData.data(), cbufData.size());
				mContext->GetCommandList()->SetComputeRootConstantBufferView(2, gpuAddr);

				commandList->Dispatch(d3d12Pass.mThreadGroupCounts[0], d3d12Pass.mThreadGroupCounts[1], d3d12Pass.mThreadGroupCounts[2]);
#if DEFERRED_EXECUTE
			});
#endif

	}

	void D3D12GraphicsRecorder::AddPreparePresent(GI::IGraphicMemoryResource* res)
	{
		auto resId = res->GetResourceId();
#if DEFERRED_EXECUTE
		mCommands.push([this, resId]()
			{
				auto res = mContext->GetDevice()->GetResourceManager()->GetResource(resId);
				res->Transition(mContext, D3D12_RESOURCE_STATE_PRESENT);
#endif
				auto devieRes = mContext->GetDevice()->GetResourceManager()->GetResource(resId);
				devieRes->Transition(mContext, D3D12_RESOURCE_STATE_PRESENT);
#if DEFERRED_EXECUTE
			}
		);
#endif
	}

	void D3D12GraphicsRecorder::AddBeginEvent(const char* mark)
	{
#if DEFERRED_EXECUTE
		mCommands.push([this, mark]()
#endif
			{
#if defined(_PIX_H_) || defined(_PIX3_H_)
				PIXBeginEvent(mContext->GetCommandList(), 0, mark);
#endif
			}
#if DEFERRED_EXECUTE
		);
#endif

	}


	void D3D12GraphicsRecorder::AddEndEvent()
	{
#if DEFERRED_EXECUTE
		mCommands.push([this]()
#endif
			{
#if defined(_PIX_H_) || defined(_PIX3_H_)
				PIXEndEvent(mContext->GetCommandList());
#endif
			}
#if DEFERRED_EXECUTE
		);
#endif
	}

	void D3D12GraphicsRecorder::Finalize(bool dropAllCommands)
	{
#if DEFERRED_EXECUTE
		while (!mCommands.empty())
		{
			if (!dropAllCommands)
				mCommands.front()();
			mCommands.pop();
		}
#endif

		mContext->Finalize();
	}

}