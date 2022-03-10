#include "RenderPch.h"
#include "D3D12PipelinePass.h"
#include "D3D12Utils.h"
#include "D3D12Resource.h"
#include "D3D12ResourceView.h"

namespace
{
	static const int32_t DescriptorTableSize = 16;
}

//////////////////////////////////////////////////////////////////////////

ComputePass::ComputePass(ComputeContext* context)
	: mContext(context)
{
	mPso = new ComputePipelineState;
}

void ComputePass::Dispatch()
{
	ID3DBlob* rsBlob = D3D12Utils::LoadRs(mRootSignatureDesc.mFile.c_str(), mRootSignatureDesc.mEntry);
	AssertHResultOk(mContext->GetDevice()->GetDevice()->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

	mPso->SetRootSignature(mRootSignature);
	mPso->SetComputeShader(CD3DX12_SHADER_BYTECODE(D3D12Utils::LoadCs(mCsFile.c_str())));
	mPso->Finalize(mContext->GetDevice()->GetPipelineStateLib());

	ID3D12GraphicsCommandList* commandList = mContext->GetCommandList();
	RuntimeDescriptorHeap* srvUavHeap = mContext->GetRuntimeHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	commandList->SetComputeRootSignature(mRootSignature);
	commandList->SetPipelineState(mPso->Get());

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srvUavHandles(DescriptorTableSize + DescriptorTableSize, mContext->GetDevice()->GetNullSrvCpuDesc().Get());
	{
		for (const auto& p : mTexParams)
		{
			if (const IShaderResourceView * srv = p.second)
			{
				srvUavHandles[p.first] = srv->GetHandle();
			}
		}

		for (const auto& p : mUavParams)
		{
			if (const IUnorderedAccessView * uav = p.second)
			{
				srvUavHandles[p.first + DescriptorTableSize] = uav->GetHandle();
			}
		}
	}
	srvUavHeap->Push(srvUavHandles.size(), srvUavHandles.data());

	ID3D12DescriptorHeap* ppHeaps[] = { srvUavHeap->GetCurrentDescriptorHeap() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetComputeRootDescriptorTable(0, srvUavHeap->GetGpuHandle(0));
	commandList->SetComputeRootDescriptorTable(1, srvUavHeap->GetGpuHandle(DescriptorTableSize));

	//srvUavHeap->Retire(srvUavHandles.size());
	commandList->Dispatch(mThreadCounts[0], mThreadCounts[1], mThreadCounts[2]);
}

//////////////////////////////////////////////////////////////////////////

GraphicsPass::GraphicsPass(GraphicsContext* context)
	: mContext(context)
{
	mPso = std::make_unique<GraphicsPipelineState>();
}

void GraphicsPass::Draw()
{
	DEBUG_PRINT("GraphicsPass::Draw");
	
	// transitions
	for (const auto& p : mSrvParams)
	{
		ID3D12Res* res = p.second->GetResource();
		res->Transition(mContext->GetCommandList(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	for (const auto& p : mRts)
	{
		ID3D12Res* res = p.second->GetResource();
		res->Transition(mContext->GetCommandList(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	//if (mDs)
	//{
	//	ID3D12Res* res = mDs->GetResource();
	//	res->Transition(graphicContext->GetCommandList(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//}

	// root signatures
	ID3DBlob* rsBlob = D3D12Utils::LoadRs(mRootSignatureDesc.mFile.c_str(), mRootSignatureDesc.mEntry);
	AssertHResultOk(mContext->GetDevice()->GetDevice()->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

	mPso->SetRootSignature(mRootSignature);

	ShaderPiece* vs = mContext->GetDevice()->GetShaderLib()->CreateVs(mVsFile.c_str());
	ShaderPiece* ps = mContext->GetDevice()->GetShaderLib()->CreatePs(mPsFile.c_str());

	mPso->SetVertexShader(CD3DX12_SHADER_BYTECODE(vs->GetShader()));
	mPso->SetPixelShader(CD3DX12_SHADER_BYTECODE(ps->GetShader()));

	const auto& inputLayout = vs->GetInputLayout();
	mPso->SetInputLayout((UINT)inputLayout.size(), inputLayout.data());

	// rts
	auto& desc = mPso->Descriptor();
	desc.NumRenderTargets = mRts.size();
	for (const auto& p : mRts)
	{
		desc.RTVFormats[p.first] = p.second->GetFormat();
		DEBUG_PRINT("RT[%d]: %s", p.first, p.second->GetResource()->GetName().c_str());
	}

	//if (mDs)
	//{
	//	desc.DSVFormat = mDs->GetFormat();
	//}

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
	srvHeap->Push(srvHandles.size(), srvHandles.data());

	ID3D12DescriptorHeap* ppHeaps[] = { srvHeap->GetCurrentDescriptorHeap() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetGraphicsRootDescriptorTable(0, srvHeap->GetGpuHandle(0));
	//srvHeap->Retire(srvHandles.size());

	// cbs
	auto BindCb = [this, commandList](ShaderPiece* shader, UINT rootParamIndex)
	{
		D3D12ConstantBuffer* cb = mContext->GetConstantBuffer();
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
				if (mCbParams.find(varName) != mCbParams.end())
				{
					const auto& varData = mCbParams[varName];
					Assert(varDesc.mSize == varData.size());
					memcpy_s(cbuf.data() + varDesc.mStartOffset, varDesc.mSize, varData.data(), varData.size());
				}
			}

			cb->Submit(cbuf.data(), cbuf.size());
		}
		commandList->SetGraphicsRootConstantBufferView(rootParamIndex, cb->GetWorkGpuVa());
		cb->Retire();
	};

	BindCb(vs, 1);
	BindCb(ps, 2);

	commandList->RSSetViewports(1, &mViewPort);
	commandList->RSSetScissorRects(1, &mScissorRect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandles[8] = {};
	for (const auto& p : mRts)
	{
		const int index = p.first;
		IRenderTargetView* rt = p.second;
		rtvHandles[index] = rt->GetHandle();
	}

	//commandList->OMSetRenderTargets(mRts.size(), rtvHandles, FALSE, mDs ? &(mDs->GetHandle()) : nullptr);
	commandList->OMSetRenderTargets(mRts.size(), rtvHandles, false, nullptr);
	commandList->OMSetStencilRef(mStencilRef);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->IASetVertexBuffers(0, mVbvs.size(), mVbvs.data());
	commandList->IASetIndexBuffer(&mIbv);

	commandList->DrawIndexedInstanced(mIndexCount, mInstanceCount, 0, 0, 0);
}

void GraphicsPass::AddSrv(const std::string& name, IShaderResourceView* srv)
{
	Assert(mSrvParams.find(name) == mSrvParams.end());
	Assert(srv);

	mSrvParams[name] = srv;
}
