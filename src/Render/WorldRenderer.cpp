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
	D3D12Device* device = mRenderModule->GetDevice();

	mSphere = D3D12Geometry::GenerateSphere(device, 3, 6);
	mQuad = D3D12Geometry::GenerateQuad(device);
	mPanoramicSkyTex = new D3D12Texture(device, R"(D:\Assets\sky0.dds)");
	
	const auto& size = renderModule->GetBackBufferSize();
	for (i32 i = 0; i < mGBufferRts.size(); ++i)
	{
		mGBufferRts[i] = new D3D12RenderTarget(device, { size.x(), size.y(), 1 }, DXGI_FORMAT_R16G16B16A16_UNORM, Utils::FormatString("GBuffer%d", i).c_str());
	}
	mDepthRt = new D3DDepthStencil(device, size,
		DXGI_FORMAT_R32_TYPELESS,
		DXGI_FORMAT_D32_FLOAT,
		DXGI_FORMAT_R32_FLOAT,
		"SceneDepthRt");
	//mDepthRt = new D3DDepthStencil(device, size,
	//	DXGI_FORMAT_R24G8_TYPELESS,
	//	DXGI_FORMAT_D24_UNORM_S8_UINT,
	//	DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
	//	"SceneDepthRt");

	mObjTrans = Translationf({ 0.f, 10.f, 0.f });
}

void WorldRenderer::TickFrame(Timer* timer)
{
	mElapsedTime = timer->GetCurrentFrameElapsedSeconds();

	f32 rotAngle = 90.f * timer->GetLastFrameDeltaTime();
	Transformf rot = Transformf(Rotationf(Math::DegreeToRadian(rotAngle), Vec3f{ 0.f, 0.f, 1.f }));
	mObjTrans = rot * mObjTrans;
}

void WorldRenderer::Render(GraphicsContext* context, IRenderTargetView* target)
{
	if (!mPanoramicSkyTex->IsD3DResourceReady())
	{
		mPanoramicSkyTex->Initial(context);
	}

	//////////////////////////////////////////////////////////////////////////
	
	{
		for (const auto& rt : mGBufferRts)
		{
			rt->Clear(context, { 0.f, 0.f, 0.f, 1.f });
		}
		mDepthRt->Clear(context, 0.f, 0);
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
			desc.DepthStencilState.DepthEnable = true;
			desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			desc.DepthStencilState.StencilEnable = false;
			desc.InputLayout = { mSphere->mInputDescs.data(), u32(mSphere->mInputDescs.size()) };
		}

		for (i32 i = 0; i < mGBufferRts.size(); ++i)
		{
			gbufferPass.mRts[i] = mGBufferRts[i]->GetRtv();
		}
		gbufferPass.mDs = mDepthRt->GetDsv();

		const Vec3i& targetSize = mGBufferRts[0]->GetSize();
		gbufferPass.mViewPort = { 0, 0, float(targetSize.x()), float(targetSize.y()) };
		gbufferPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

		gbufferPass.mVbvs.clear();
		gbufferPass.mVbvs.push_back(mSphere->mVbv);
		gbufferPass.mIbv = mSphere->mIbv;
		gbufferPass.mIndexCount = mSphere->mIbv.SizeInBytes / sizeof(u16);

		gbufferPass.AddCbVar("time", mElapsedTime);
		gbufferPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
		
		gbufferPass.AddCbVar("worldMat", mObjTrans.matrix());
		gbufferPass.AddCbVar("viewMat", Math::ComputeViewMatrix({}, mDir, mUp, mRight));
		gbufferPass.AddCbVar("projMat", mCameraProj.ComputeProjectionMatrix());

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
		lightingPass.AddSrv("GBuffer0", mGBufferRts[0]->GetSrv());
		lightingPass.AddSrv("GBuffer1", mGBufferRts[1]->GetSrv());
		lightingPass.AddSrv("GBuffer2", mGBufferRts[2]->GetSrv());

		lightingPass.AddSrv("PanoramicSky", mPanoramicSkyTex->GetSrv());

		lightingPass.Draw();
	}
}
