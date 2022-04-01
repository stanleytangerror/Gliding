#include "RenderPch.h"
#include "D3D12PipelinePass.h"
#include "D3D12Utils.h"
#include "D3D12Resource.h"
#include "D3D12ResourceView.h"

namespace
{
	D3D12_GPU_VIRTUAL_ADDRESS BindConstBufferParams(const std::map<std::string, std::vector<byte>>& cbParams, D3D12CommandContext* context, ShaderPiece* shader)
	{
		D3D12ConstantBuffer* cb = context->GetConstantBuffer();
		std::map<std::string, InputCBufferParam> cbufBindings;

		for (const auto& p : shader->GetCBufferBindings())
		{
			if (cbufBindings.find(p.first) == cbufBindings.end())
			{
				cbufBindings[p.first] = p.second;
			}
		}
		for (const auto& p : shader->GetCBufferBindings())
		{
			const InputCBufferParam& cbParam = p.second;
			std::vector<byte> cbuf;
			cbuf.resize(cbParam.mSize, 0);

			for (const auto& q : cbParam.mVariables)
			{
				const std::string& varName = q.first;
				const InputCBufferParam::CBufferVar& varDesc = q.second;
				if (cbParams.find(varName) != cbParams.end())
				{
					const auto& varData = cbParams.find(varName)->second;
					Assert(varDesc.mSize == varData.size());
					memcpy_s(cbuf.data() + varDesc.mStartOffset, varDesc.mSize, varData.data(), varData.size());
				}
			}

			cb->Submit(cbuf.data(), static_cast<i32>(cbuf.size()));
		}
		D3D12_GPU_VIRTUAL_ADDRESS gpuVa = cb->GetWorkGpuVa();
		cb->Retire();
		return gpuVa;
	};

	template <typename T, typename V>
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> BindSrvUavParams(D3D12CommandContext* context, const std::map<std::string, T>& paramBindings, const std::map<std::string, V>& paramValues)
	{
		int maxIdx = 0;
		for (const auto& p : paramBindings)
		{
			const T& param = p.second;
			maxIdx = std::max<int>(maxIdx, param.mBindPoint);
		}

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(maxIdx + 1, context->GetDevice()->GetNullSrvCpuDesc().Get());
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
	ShaderPiece* cs = mContext->GetDevice()->GetShaderLib()->CreateCs(mCsFile.c_str());
	mPso->SetComputeShader(CD3DX12_SHADER_BYTECODE(cs->GetShader()));

	mPso->Finalize(mContext->GetDevice()->GetPipelineStateLib());

	ID3D12GraphicsCommandList* commandList = mContext->GetCommandList();
	RuntimeDescriptorHeap* srvUavHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	commandList->SetComputeRootSignature(mRootSignature);
	commandList->SetPipelineState(mPso->Get());

	// srv uav
	{
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& srvHandles = BindSrvUavParams(mContext, cs->GetSrvBindings(), mSrvParams);

		const auto& gpuDescBase = srvUavHeap->Push(static_cast<i32>(srvHandles.size()), srvHandles.data());

		ID3D12DescriptorHeap* ppHeaps[] = { srvUavHeap->GetCurrentDescriptorHeap() };
		commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		commandList->SetComputeRootDescriptorTable(0, gpuDescBase);
	}
	
	{
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& uavHandles = BindSrvUavParams(mContext, cs->GetUavBindings(), mUavParams);

		const auto& gpuDescBase = srvUavHeap->Push(static_cast<i32>(uavHandles.size()), uavHandles.data());

		ID3D12DescriptorHeap* ppHeaps[] = { srvUavHeap->GetCurrentDescriptorHeap() };
		commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		commandList->SetComputeRootDescriptorTable(1, gpuDescBase);
	}

	mContext->GetCommandList()->SetComputeRootConstantBufferView(2, BindConstBufferParams(mCbParams, mContext, cs));

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

//////////////////////////////////////////////////////////////////////////

GraphicsPass::GraphicsPass(GraphicsContext* context)
	: mContext(context)
{
	mPso = std::make_unique<GraphicsPipelineState>();
}

void GraphicsPass::Draw()
{
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
	ShaderPiece* vs = mContext->GetDevice()->GetShaderLib()->CreateVs(mVsFile.c_str());
	ShaderPiece* ps = mContext->GetDevice()->GetShaderLib()->CreatePs(mPsFile.c_str());

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
	RuntimeDescriptorHeap* srvHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	commandList->SetGraphicsRootSignature(mRootSignature);
	commandList->SetPipelineState(mPso->Get());

	// srvs
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

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srvHandles(maxSrvIndex + 1, mContext->GetDevice()->GetNullSrvCpuDesc().Get());
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

	ID3D12DescriptorHeap* ppHeaps[] = { srvHeap->GetCurrentDescriptorHeap() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetGraphicsRootDescriptorTable(0, gpuDescBaseAddr);

	// cbs
	mContext->GetCommandList()->SetGraphicsRootConstantBufferView(1, BindConstBufferParams(mCbParams, mContext, vs));
	mContext->GetCommandList()->SetGraphicsRootConstantBufferView(2, BindConstBufferParams(mCbParams, mContext, ps));

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
