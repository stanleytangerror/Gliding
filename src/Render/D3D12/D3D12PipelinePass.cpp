#include "RenderPch.h"
#include "D3D12PipelinePass.h"
#include "D3D12Utils.h"
#include "D3D12Resource.h"
#include "D3D12ResourceView.h"

namespace
{
	i32 BindConstBufferParams(std::vector<byte>& cbuf, const std::map<std::string, std::vector<byte>>& cbArgs, ShaderPiece* shader)
	{
		const std::vector<InputCBufferParam>& cbufBindings = shader->GetCBufferBindings();
		const i32 cbSize = std::accumulate(cbufBindings.begin(), cbufBindings.end(), 0,
			[](i32 size, const auto& param) { return size + param.mSize; });

		i32 offset = cbuf.size();
		cbuf.insert(cbuf.end(), cbSize, 0);
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
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> BindSrvUavParams(D3D12CommandContext* context, const std::map<std::string, T>& paramBindings, const std::map<std::string, V>& paramValues, const CpuDescItem& nullDesc)
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
				handles[param.mBindPoint] = paramValues.find(name)->second->GetHandle();
			}
		}
		return handles;
	}
}

//////////////////////////////////////////////////////////////////////////

ComputePass::ComputePass(GraphicsContext* context)
	: mContext(context)
{
	mPso = std::make_unique<ComputePipelineState>();
}

void ComputePass::Dispatch()
{
	for (const auto& p : mSrvParams)
	{
		ID3D12Res* res = p.second->GetResource();
		res->Transition(mContext, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	for (const auto& p : mUavParams)
	{
		ID3D12Res* res = p.second->GetResource();
		res->Transition(mContext, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	// root signatures
	D3D12PipelineStateLibrary* psoLib = mContext->GetDevice()->GetPipelineStateLib();
	mRootSignature = psoLib->CreateRootSignature(mRootSignatureDesc.mFile.c_str(), mRootSignatureDesc.mEntry);

	mPso->SetRootSignature(mRootSignature);

	// shader
	ShaderPiece* cs = mContext->GetDevice()->GetShaderLib()->CreateCs(mCsFile.c_str(), mShaderMacros);
	mPso->SetComputeShader(CD3DX12_SHADER_BYTECODE(cs->GetShader()));

	mPso->Finalize(mContext->GetDevice()->GetPipelineStateLib());

	ID3D12GraphicsCommandList* commandList = mContext->GetCommandList();

	commandList->SetComputeRootSignature(mRootSignature);
	commandList->SetPipelineState(mPso->Get());

	std::set<ID3D12DescriptorHeap*> heaps;
	std::map<i32, CD3DX12_GPU_DESCRIPTOR_HANDLE> gpuBaseAddrs;


	// srv
	{
		RuntimeDescriptorHeap* srvUavHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srvHandles = BindSrvUavParams(mContext, cs->GetSrvBindings(), mSrvParams, mContext->GetDevice()->GetNullSrvUavCbvCpuDesc());

		const auto& gpuDescBase = srvUavHeap->Push(static_cast<i32>(srvHandles.size()), srvHandles.data());

		heaps.insert(srvUavHeap->GetCurrentDescriptorHeap());
		gpuBaseAddrs[0] = gpuDescBase;
	}
	
	// uav
	{
		RuntimeDescriptorHeap* srvUavHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& uavHandles = BindSrvUavParams(mContext, cs->GetUavBindings(), mUavParams, mContext->GetDevice()->GetNullSrvUavCbvCpuDesc());

		const auto& gpuDescBase = srvUavHeap->Push(static_cast<i32>(uavHandles.size()), uavHandles.data());

		heaps.insert(srvUavHeap->GetCurrentDescriptorHeap());
		gpuBaseAddrs[1] = gpuDescBase;
	}

	RuntimeDescriptorHeap* samplerHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	{
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& samplerHandles = BindSrvUavParams(mContext, cs->GetSamplerBindings(), mSamplerParams, mContext->GetDevice()->GetNullSamplerCpuDesc());

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

	std::vector<byte> cbufData;
	BindConstBufferParams(cbufData, mCbParams, cs);
	const D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = mContext->GetConstantBuffer()->Push(cbufData.data(), cbufData.size());
	mContext->GetCommandList()->SetComputeRootConstantBufferView(2, gpuAddr);

	commandList->Dispatch(mThreadGroupCounts[0], mThreadGroupCounts[1], mThreadGroupCounts[2]);
}

void ComputePass::AddSrv(const std::string& name, IShaderResourceView* srv)
{
	Assert(mSrvParams.find(name) == mSrvParams.end());
	Assert(srv);

	mSrvParams[name] = srv;
}

void ComputePass::AddUav(const std::string& name, IUnorderedAccessView* uav)
{
	Assert(mUavParams.find(name) == mUavParams.end());
	Assert(uav);

	mUavParams[name] = uav;
}

void ComputePass::AddSampler(const std::string& name, D3D12SamplerView* sampler)
{
	Assert(mSamplerParams.find(name) == mSamplerParams.end());
	Assert(sampler);

	mSamplerParams[name] = sampler;
}

//////////////////////////////////////////////////////////////////////////

GraphicsPass::GraphicsPass(GraphicsContext* context)
	: mContext(context)
{
	mPso = std::make_unique<GraphicsPipelineState>();
}

void GraphicsPass::Draw()
{
	PROFILE_EVENT(GraphicsPass::Draw);

	// transitions
	for (const auto& p : mSrvParams)
	{
		ID3D12Res* res = p.second->GetResource();
		res->Transition(mContext, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	for (const auto& p : mRts)
	{
		ID3D12Res* res = p.second->GetResource();
		res->Transition(mContext, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	if (mDs)
	{
		ID3D12Res* res = mDs->GetResource();
		res->Transition(mContext, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	// root signatures
	D3D12PipelineStateLibrary* psoLib = mContext->GetDevice()->GetPipelineStateLib();
	mRootSignature = psoLib->CreateRootSignature(mRootSignatureDesc.mFile.c_str(), mRootSignatureDesc.mEntry);

	mPso->SetRootSignature(mRootSignature);

	// shader
	ShaderPiece* vs = mContext->GetDevice()->GetShaderLib()->CreateVs(mVsFile.c_str(), mShaderMacros);
	ShaderPiece* ps = mContext->GetDevice()->GetShaderLib()->CreatePs(mPsFile.c_str(), mShaderMacros);

	mPso->SetVertexShader(CD3DX12_SHADER_BYTECODE(vs->GetShader()));
	mPso->SetPixelShader(CD3DX12_SHADER_BYTECODE(ps->GetShader()));

	const auto& inputLayout = vs->GetInputLayout();
	mPso->SetInputLayout((UINT)inputLayout.size(), inputLayout.data());

	// rts
	auto& desc = mPso->Descriptor();
	desc.NumRenderTargets = static_cast<u32>(mRts.size());
	for (const auto& p : mRts)
	{
		desc.RTVFormats[p.first] = p.second->GetFormat();
	}

	if (mDs)
	{
		desc.DSVFormat = mDs->GetFormat();
	}

	mPso->Finalize(mContext->GetDevice()->GetPipelineStateLib());

	// resource bindings
	ID3D12GraphicsCommandList* commandList = mContext->GetCommandList();

	commandList->SetGraphicsRootSignature(mRootSignature);
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

			if (mSrvParams.find(srvName) != mSrvParams.end())
			{
				srvHandles[srvParam.mBindPoint] = mSrvParams[srvName]->GetHandle();
			}
		}
		const auto& gpuDescBaseAddr = srvHeap->Push(static_cast<i32>(srvHandles.size()), srvHandles.data());

		heaps.insert(srvHeap->GetCurrentDescriptorHeap());
		gpuBaseAddrs[0] = gpuDescBaseAddr;
	}

	{
		RuntimeDescriptorHeap* samplerHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& samplerHandles = BindSrvUavParams(mContext, ps->GetSamplerBindings(), mSamplerParams, mContext->GetDevice()->GetNullSamplerCpuDesc());

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
	std::vector<byte> cbufData;
	BindConstBufferParams(cbufData, mCbParams, vs);
	BindConstBufferParams(cbufData, mCbParams, ps);
	const D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = mContext->GetConstantBuffer()->Push(cbufData.data(), cbufData.size());
	mContext->GetCommandList()->SetGraphicsRootConstantBufferView(1, gpuAddr);

	Assert(mViewPort.MinDepth < mViewPort.MaxDepth);
	commandList->RSSetViewports(1, &mViewPort);
	commandList->RSSetScissorRects(1, &mScissorRect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandles[8] = {};
	for (const auto& p : mRts)
	{
		const int index = p.first;
		IRenderTargetView* rt = p.second;
		rtvHandles[index] = rt->GetHandle();
	}

	if (mDs)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsHandle = mDs->GetHandle();
		commandList->OMSetRenderTargets(static_cast<i32>(mRts.size()), rtvHandles, false, &dsHandle);
	}
	else
	{
		commandList->OMSetRenderTargets(static_cast<i32>(mRts.size()), rtvHandles, false, nullptr);
	}
	commandList->OMSetStencilRef(mStencilRef);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->IASetVertexBuffers(0, static_cast<u32>(mVbvs.size()), mVbvs.data());
	commandList->IASetIndexBuffer(&mIbv);

	commandList->DrawIndexedInstanced(mIndexCount, mInstanceCount, 0, 0, 0);
}

void GraphicsPass::AddSrv(const std::string& name, IShaderResourceView* srv)
{
	Assert(mSrvParams.find(name) == mSrvParams.end());
	Assert(srv);

	mSrvParams[name] = srv;
}

void GraphicsPass::AddSampler(const std::string& name, D3D12SamplerView* sampler)
{
	Assert(mSamplerParams.find(name) == mSamplerParams.end());
	Assert(sampler);

	mSamplerParams[name] = sampler;
}
