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
#include "World/Scene.h"

WorldRenderer::WorldRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	D3D12Device* device = mRenderModule->GetDevice();

	mSphere = D3D12Geometry::GenerateSphere(device, 20, 40);
	mQuad = D3D12Geometry::GenerateQuad(device);
	mPanoramicSkyTex = new D3D12Texture(device, R"(D:\Assets\Panorama_of_Marienplatz.dds)");
	
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

	mObjTrans = Translationf({ 0.f, 5.f, 0.f });

	mGismo.mContent = nullptr;
	mGismo.mChildren.push_back(TransformNode<D3D12Geometry*>{ mSphere, Transformf(Translationf(1.f, 0.f, 0.f)) * Transformf(Scalingf(1.f, 0.1f, 0.1f)) });
	mGismo.mChildren.push_back(TransformNode<D3D12Geometry*>{ mSphere, Transformf(Translationf(0.f, 1.f, 0.f)) * Transformf(Scalingf(0.1f, 1.f, 0.1f)) });
	mGismo.mChildren.push_back(TransformNode<D3D12Geometry*>{ mSphere, Transformf(Translationf(0.f, 0.f, 1.f)) * Transformf(Scalingf(0.1f, 0.1f, 1.f)) });

	SceneRawData* sceneRawData = SceneRawData::LoadScene(R"(res/Scene/sphere.obj)");
	mSphereMesh = D3D12Geometry::GenerateGeometryFromMeshRawData(device, sceneRawData->mModels.front());
}

WorldRenderer::~WorldRenderer()
{
	Utils::SafeDelete(mQuad);
	Utils::SafeDelete(mSphere);
	Utils::SafeDelete(mSphereMesh);
	Utils::SafeDelete(mPanoramicSkyTex);
	Utils::SafeDelete(mDepthRt);
	for (auto& rt : mGBufferRts)
	{
		Utils::SafeDelete(rt);
	}
}

void WorldRenderer::TickFrame(Timer* timer)
{
	mElapsedTime = timer->GetCurrentFrameElapsedSeconds();

	f32 rotAngle = 90.f * timer->GetLastFrameDeltaTime();
	Transformf rot = Transformf(Rotationf(Math::DegreeToRadian(rotAngle), Vec3f{ 0.f, 0.f, 1.f }));
	mObjTrans = rot * mObjTrans;

	mGismo.CalcAbsTransform();
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
		mDepthRt->Clear(context, mCameraProj.GetFarPlaneDepth(), 0);
	}

	mGismo.ForEach([&](const TransformNode<D3D12Geometry*>& node) 
		{
			if (node.mContent)
			{
				RenderGeometry(context, node.mContent, node.mAbsTransform);
			}
		});

	RenderGeometry(context, mSphereMesh, mObjTrans);

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
		lightingPass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
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

	RenderSky(context, target, mDepthRt->GetDsv());
}

void WorldRenderer::RenderGeometry(GraphicsContext* context, D3D12Geometry* geometry, const Transformf& transform) const
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
		desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { geometry->mInputDescs.data(), u32(geometry->mInputDescs.size()) };
	}

	for (i32 i = 0; i < mGBufferRts.size(); ++i)
	{
		gbufferPass.mRts[i] = mGBufferRts[i]->GetRtv();
	}
	gbufferPass.mDs = mDepthRt->GetDsv();

	const Vec3i& targetSize = mGBufferRts[0]->GetSize();
	gbufferPass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	gbufferPass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

	gbufferPass.mVbvs.clear();
	gbufferPass.mVbvs.push_back(geometry->mVbv);
	gbufferPass.mIbv = geometry->mIbv;
	gbufferPass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	gbufferPass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

	gbufferPass.AddCbVar("worldMat", transform.matrix());
	gbufferPass.AddCbVar("viewMat", Math::ComputeViewMatrix(mCamPos, mDir, mUp, mRight));
	gbufferPass.AddCbVar("projMat", mCameraProj.ComputeProjectionMatrix());

	gbufferPass.Draw();
}

void WorldRenderer::RenderSky(GraphicsContext* context, IRenderTargetView* target, DSV* depth) const
{
	GraphicsPass pass(context);

	D3D12Geometry* geometry = mSphere;
	const Transformf& transform = Transformf(UniScalingf(1000.f));

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/PanoramicSky.hlsl";
	pass.mPsFile = "res/Shader/PanoramicSky.hlsl";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = true;
		desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { geometry->mInputDescs.data(), u32(geometry->mInputDescs.size()) };
	}

	pass.mRts[0] = target;
	pass.mDs = depth;
	const Vec3i& targetSize = target->GetResource()->GetSize();
	pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	pass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->mVbv);
	pass.mIbv = geometry->mIbv;
	pass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });

	pass.AddCbVar("worldMat", transform.matrix());
	pass.AddCbVar("viewMat", Math::ComputeViewMatrix(mCamPos, mDir, mUp, mRight));
	pass.AddCbVar("projMat", mCameraProj.ComputeProjectionMatrix());

	pass.AddSrv("PanoramicSky", mPanoramicSkyTex->GetSrv());

	pass.Draw();
}
