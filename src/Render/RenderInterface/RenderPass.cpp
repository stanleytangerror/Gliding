#include "RenderPch.h"
#include "RenderInterface/RenderPass.h"
#include "RenderResource.h"
#include "RenderModule.h"
#include "D3D12Backend/D3D12Headers.h"
#include "D3D12Backend/D3D12Device.h"
#include "D3D12Backend/D3D12PipelinePass.h"
#include "D3D12Backend/D3D12ResourceView.h"
#include "D3D12Backend/D3D12Resource.h"
#include "D3D12Backend/D3D12CommandContext.h"
#include "D3D12Backend/D3D12ResourceView.h"
#include "RenderTypeD3D12Utils.h"

void RenderPassManager::ParseAllPassses()
{
	std::vector<RenderPass> thisFramePasses;
	std::swap(thisFramePasses, mPasses);

	D3D12Backend::GraphicsContext* context = mRenderModule->GetDevice()->GetGpuQueue(D3D12Backend::D3D12GpuQueueType::Graphic)->AllocGraphicContext();
	RenderResourceManager* resMgr = mRenderModule->GetRenderResourceManager();

	for (const RenderPass& pass : thisFramePasses)
	{
		RENDER_EVENT_STR(context, pass.Name.c_str());

		D3D12Backend::GraphicsPass devicePass(context);

		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements(pass.InputPrimitive.InputElements.size());
		std::transform(pass.InputPrimitive.InputElements.begin(), pass.InputPrimitive.InputElements.end(), inputElements.begin(), ToInputElementDesc);

		devicePass.mRootSignatureDesc.mFile = pass.Program.RsName;
		devicePass.mRootSignatureDesc.mEntry = pass.Program.RsEntry.c_str();
		devicePass.mVsFile = pass.Program.VsName;
		devicePass.mPsFile = pass.Program.PsName;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = devicePass.PsoDesc();
		{
			desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			desc.DepthStencilState.DepthEnable = false;
			desc.DepthStencilState.StencilEnable = false;
			desc.InputLayout = { inputElements.data(), (u32) inputElements.size() };
		}

		for (const auto& [name, value] : pass.ConstBuffers)
		{
			devicePass.AddCbVar(name.c_str(), value);
		}

		devicePass.mViewPort = ToViewPort(pass.ViewPort);
		devicePass.mScissorRect = ToRect(pass.ScissorRect);

		for (const auto& [name, srv] : pass.Srvs)
		{
			ViewId viewId = resMgr->CreateSrv(srv.ResourceId, srv.SrvDesc);
			devicePass.AddSrv(name.c_str(), resMgr->GetSrv(viewId));
		}

		for (auto i = 0; i < pass.Rtvs.size(); ++i)
		{
			const auto& rtv = pass.Rtvs[i];
			ViewId viewId = resMgr->CreateRtv(rtv.ResourceId, rtv.RtvDesc);

			devicePass.mRts[i] = resMgr->GetRtv(viewId);
		}

		devicePass.mVbvs.resize(pass.InputPrimitive.Vbvs.size());
		std::transform(pass.InputPrimitive.Vbvs.begin(), pass.InputPrimitive.Vbvs.end(),
			devicePass.mVbvs.begin(), [resMgr](const VertexBufferView& v)
			{
				D3D12Backend::CommitedResource* resource = resMgr->GetNamedReadonlyResource(v.ResourceId);
				auto location = resource->GetD3D12Resource()->GetGPUVirtualAddress() + v.Desc.Offset;
				return D3D12_VERTEX_BUFFER_VIEW{ v.Desc.Size, v.Desc.Size, v.Desc.Stride };
			});
		
		const auto& ibv = pass.InputPrimitive.Ibv;
		D3D12Backend::CommitedResource* ibRes = resMgr->GetNamedReadonlyResource(ibv.ResourceId);
		auto ibLocation = ibRes->GetD3D12Resource()->GetGPUVirtualAddress() + ibv.Desc.Offset;
		devicePass.mIbv = D3D12_INDEX_BUFFER_VIEW{ ibLocation, ibv.Desc.Size, ToDxgiFormat(ibv.Desc.Format) };

		devicePass.mIndexCount = pass.InputPrimitive.Param.IndexCount;
		devicePass.mInstanceCount = pass.InputPrimitive.Param.InstanceCount;

		devicePass.Draw();
	}
}
