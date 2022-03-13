#include "RenderPch.h"
#include "WorldRenderer.h"
#include "RenderModule.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12PipelinePass.h"
#include "D3D12/D3D12RenderTarget.h"
#include "D3D12/D3D12ResourceView.h"
#include "D3D12/D3D12Resource.h"
#include "D3D12/D3D12Geometry.h"
#include "D3D12/D3D12Texture.h"

WorldRenderer::WorldRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	mSphere = D3D12Geometry::GenerateSphere(mRenderModule->GetDevice(), 20, 40);
	mQuad = D3D12Geometry::GenerateQuad(mRenderModule->GetDevice());
	mTex = new D3D12Texture(mRenderModule->GetDevice(), R"(res/Texture/bluecloud_dn.dds)");
	
	for (i32 i = 0; i < mGBufferRts.size(); ++i)
	{
		mGBufferRts[i] = new D3D12RenderTarget(renderModule->GetDevice(), { 256, 256, 1 }, DXGI_FORMAT_R16G16B16A16_UNORM, Utils::FormatString("GBuffer%d", i).c_str());
	}
}

void WorldRenderer::TickFrame(Timer* timer)
{
	mElapsedTime = timer->GetCurrentFrameElapsedSeconds();
}

void WorldRenderer::Render(GraphicsContext* context, IRenderTargetView* target)
{
	if (!mTex->IsD3DResourceReady())
	{
		mTex->Initial(context);
	}

	//////////////////////////////////////////////////////////////////////////
	
	{
		for (const auto& rt : mGBufferRts)
		{
			rt->Clear(context, { 0.f, 0.f, 0.f, 1.f });
		}
	}

	{
		GraphicsPass gbufferPass(context);

		gbufferPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
		gbufferPass.mRootSignatureDesc.mEntry = "GraphicsRS";
		gbufferPass.mVsFile = "res/Shader/GBuffer.hlsl";
		gbufferPass.mPsFile = "res/Shader/GBuffer.hlsl";

		D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = gbufferPass.PsoDesc();
		{
			desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			desc.DepthStencilState.DepthEnable = false;
			desc.DepthStencilState.StencilEnable = false;
			desc.InputLayout = { mSphere->mInputDescs.data(), u32(mSphere->mInputDescs.size()) };
		}

		for (i32 i = 0; i < mGBufferRts.size(); ++i)
		{
			gbufferPass.mRts[i] = mGBufferRts[i]->GetRtv();
		}
		const Vec3i& targetSize = mGBufferRts[0]->GetSize();
		gbufferPass.mViewPort = { 0, 0, float(targetSize.x()), float(targetSize.y()) };
		gbufferPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

		gbufferPass.mVbvs.clear();
		gbufferPass.mVbvs.push_back(mSphere->mVbv);
		gbufferPass.mIbv = mSphere->mIbv;
		gbufferPass.mIndexCount = mSphere->mIbv.SizeInBytes / sizeof(u16);

		gbufferPass.AddCbVar("time", mElapsedTime);
		gbufferPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

		gbufferPass.Draw();
	}

	//////////////////////////////////////////////////////////////////////////

	{
		GraphicsPass lightingPass(context);

		lightingPass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
		lightingPass.mRootSignatureDesc.mEntry = "GraphicsRS";
		lightingPass.mVsFile = "res/Shader/Lighting.hlsl";
		lightingPass.mPsFile = "res/Shader/Lighting.hlsl";

		D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = lightingPass.PsoDesc();
		{
			desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			desc.DepthStencilState.DepthEnable = false;
			desc.DepthStencilState.StencilEnable = false;
			desc.InputLayout = { mQuad->mInputDescs.data(), u32(mQuad->mInputDescs.size()) };
		}

		const Vec3i& targetSize = target->GetResource()->GetSize();
		lightingPass.mRts[0] = target;
		lightingPass.mViewPort = { 0, 0, float(targetSize.x()), float(targetSize.y()) };
		lightingPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

		lightingPass.mVbvs.clear();
		lightingPass.mVbvs.push_back(mQuad->mVbv);
		lightingPass.mIbv = mQuad->mIbv;
		lightingPass.mIndexCount = mQuad->mIbv.SizeInBytes / sizeof(u16);

		lightingPass.AddCbVar("time", mElapsedTime);
		lightingPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
		lightingPass.AddSrv("InputTex0", mTex->GetSrv());

		lightingPass.Draw();
	}
}
