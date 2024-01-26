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
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();

#if DEFERRED_EXECUTE
		mCommands.push([this, resourceManager, pass]()
			{
#endif
				for (const auto& vbv : pass.mVbvs)
				{
					if (!vbv.GetResourceId()) { return; }
				}

				std::map<std::string, DescriptorPtr> srvs;
				for (const auto& [name, srv] : pass.mSrvParams)
				{
					if (!srv.GetResourceId()) { return; }
					srvs[name] = resourceManager->CreateSrvDescriptor(srv.GetResourceId(), srv);
				}

				std::vector<DescriptorPtr> rtvs;
				std::vector<DXGI_FORMAT> rtvFormats;
				auto numRtvs = std::count_if(pass.mRtvs.begin(), pass.mRtvs.end(), [](const auto& rtv) { return rtv.GetResourceId(); });
				if (numRtvs == 0 && !pass.mDsv.GetResourceId()) { return; }
				for (auto i = 0; i < numRtvs; ++i)
				{
					rtvs.push_back(resourceManager->CreateRtvDescriptor(pass.mRtvs[i].GetResourceId(), pass.mRtvs[i]));
					rtvFormats.push_back(D3D12Utils::ToDxgiFormat(pass.mRtvs[i].GetFormat()));
				}

				std::map<std::string, DescriptorPtr> samplers;
				for (const auto& [name, sampler] : pass.mSamplerParams)
				{
					samplers[name] = resourceManager->CreateSampler(sampler);
				}


				// transitions
				for (const auto& [_, srv] : pass.mSrvParams)
				{
					auto res = resourceManager->GetResource(srv.GetResourceId());
					res->Transition(mContext, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				}

				for (const auto& rtv : pass.mRtvs)
				{
					if (rtv.GetResourceId())
					{
						auto res = resourceManager->GetResource(rtv.GetResourceId());
						res->Transition(mContext, D3D12_RESOURCE_STATE_RENDER_TARGET);
					}
				}

				if (pass.mDsv.GetResourceId())
				{
					auto res = resourceManager->GetResource(pass.mDsv.GetResourceId());
					res->Transition(mContext, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				}

				// root signatures
				D3D12PipelineStateLibrary* psoLib = mContext->GetDevice()->GetPipelineStateLib();
				auto rootSignature = psoLib->CreateRootSignature(pass.mRootSignatureDesc.mFile.c_str(), pass.mRootSignatureDesc.mEntry);

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
				auto& desc = pso->Descriptor();
				desc.NumRenderTargets = numRtvs;
				for (auto i = 0; i < sizeof(desc.RTVFormats) / sizeof(desc.RTVFormats[0]); ++i)
				{
					desc.RTVFormats[i] = i < numRtvs ? rtvFormats[i] : DXGI_FORMAT_UNKNOWN;
				}

				if (pass.mDsv.GetResourceId())
				{
					desc.DSVFormat = D3D12Utils::ToDxgiFormat(pass.mDsv.GetFormat());
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
				for (; rtvCount < rtvs.size(); ++rtvCount)
				{
					if (!rtvs[rtvCount]) { break; }
					rtvHandles[rtvCount] = rtvs[rtvCount].Get();
				}

				if (pass.mDsv.GetResourceId())
				{
					CD3DX12_CPU_DESCRIPTOR_HANDLE dsHandle = resourceManager->CreateDsvDescriptor(pass.mDsv.GetResourceId(), pass.mDsv).Get();
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
					vbvs[i].BufferLocation = resourceManager->GetResource(pass.mVbvs[i].GetResourceId())->GetD3D12Resource()->GetGPUVirtualAddress();
					vbvs[i].SizeInBytes = pass.mVbvs[i].GetSizeInBytes();
					vbvs[i].StrideInBytes = pass.mVbvs[i].GetStrideInBytes();
				}
				commandList->IASetVertexBuffers(0, static_cast<u32>(vbvs.size()), vbvs.data());

				D3D12_INDEX_BUFFER_VIEW ibv;
				{
					ibv.BufferLocation = resourceManager->GetResource(pass.mIbv.GetResourceId())->GetD3D12Resource()->GetGPUVirtualAddress();
					ibv.SizeInBytes = pass.mIbv.GetSizeInBytes();
					ibv.Format = D3D12Utils::ToDxgiFormat(pass.mIbv.GetFormat());
				}
				commandList->IASetIndexBuffer(&ibv);

				commandList->DrawIndexedInstanced(pass.mIndexCount, pass.mInstanceCount, pass.mIndexStartLocation, pass.mVertexStartLocation, 0);
#if DEFERRED_EXECUTE
			});
#endif
	}

	void D3D12GraphicsRecorder::AddComputePass(const GI::ComputePass& pass)
	{
		ResourceManager* resourceManager = mContext->GetDevice()->GetResourceManager();

#if DEFERRED_EXECUTE
		mCommands.push([this, resourceManager, pass]()
			{
#endif
				std::map<std::string, DescriptorPtr> srvs;
				for (const auto& [name, srv] : pass.mSrvParams)
				{
					if (!srv.GetResourceId()) { return; }
					srvs[name] = resourceManager->CreateSrvDescriptor(srv.GetResourceId(), srv);
				}

				std::map<std::string, DescriptorPtr> uavs;
				for (const auto& [name, uav] : pass.mUavParams)
				{
					if (!uav.GetResourceId()) { return; }
					uavs[name] = resourceManager->CreateUavDescriptor(uav.GetResourceId(), uav);
				}

				std::map<std::string, DescriptorPtr> samplers;
				for (const auto& [name, sampler] : pass.mSamplerParams)
				{
					samplers[name] = resourceManager->CreateSampler(sampler);
				}

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
				auto rootSignature = psoLib->CreateRootSignature(pass.mRootSignatureDesc.mFile.c_str(), pass.mRootSignatureDesc.mEntry);

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
				BindConstBufferParams(cbufData, pass.mCbParams, cs);
				const D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = mContext->GetConstantBuffer()->Push(cbufData.data(), cbufData.size());
				mContext->GetCommandList()->SetComputeRootConstantBufferView(2, gpuAddr);

				commandList->Dispatch(pass.mThreadGroupCounts[0], pass.mThreadGroupCounts[1], pass.mThreadGroupCounts[2]);
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