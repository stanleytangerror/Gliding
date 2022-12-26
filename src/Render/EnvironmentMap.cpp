#include "RenderPch.h"
#include "EnvironmentMap.h"
#include "D3D12Backend/D3D12RenderTarget.h"
#include "D3D12Backend/D3D12PipelinePass.h"
#include "D3D12Backend/D3D12CommandContext.h"
#include "D3D12Backend/D3D12Geometry.h"

D3D12RenderTarget* EnvironmentMap::GenerateIrradianceMap(GraphicsContext* context, IShaderResourceView* sky, i32 resolution, i32 semiSphereBusbarSampleCount)
{
	static D3D12SamplerView* mPanoramicSkySampler = new D3D12SamplerView(context->GetDevice(), D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	static D3D12Geometry* mQuad = D3D12Geometry::GenerateQuad(context->GetDevice());

	const Vec2i& rtSize = { resolution * 2, resolution };
	D3D12RenderTarget* irradianceMap = new D3D12RenderTarget(context->GetDevice(), Vec3i{ rtSize.x(), rtSize.y(), 1 }, DXGI_FORMAT_R32G32B32A32_FLOAT, "IrradianceMap");

	RENDER_EVENT(context, GenerateIrradianceMap);

	GraphicsPass pass(context);

	D3D12Geometry* geometry = mQuad;
	const Transformf& transform = Transformf(UniScalingf(1000.f));

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mPsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mShaderMacros.push_back(ShaderMacro{ "GENERATE_IRRADIANCE_MAP", "1" });

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { geometry->mInputDescs.data(), u32(geometry->mInputDescs.size()) };
	}

	pass.mRts[0] = irradianceMap->GetRtv();
	pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(rtSize.x()), float(rtSize.y()));
	pass.mScissorRect = { 0, 0, rtSize.x(), rtSize.y() };
	pass.mStencilRef = 0;

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->mVbv);
	pass.mIbv = geometry->mIbv;
	pass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("RtSize", Vec4f{ f32(rtSize.x()), f32(rtSize.y()), 1.f / rtSize.x(), 1.f / rtSize.y() });

	const f32 deltaRad = Math::Pi<f32>() / 2.f / semiSphereBusbarSampleCount;
	const f32 sampleCount = semiSphereBusbarSampleCount * semiSphereBusbarSampleCount * 4.f;
	pass.AddCbVar("SemiSphereSampleInfo", Vec4f{ deltaRad, sampleCount, 1.f / sampleCount, 0.f });

	pass.AddSrv("PanoramicSky", sky);
	pass.AddSampler("PanoramicSkySampler", mPanoramicSkySampler);

	pass.Draw();

	return irradianceMap;
}

D3D12RenderTarget* EnvironmentMap::GenerateIntegratedBRDF(GraphicsContext* context, i32 resolution)
{
	static D3D12SamplerView* mPanoramicSkySampler = new D3D12SamplerView(context->GetDevice(), D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	static D3D12Geometry* mQuad = D3D12Geometry::GenerateQuad(context->GetDevice());

	const Vec2i& rtSize = { resolution, resolution };
	D3D12RenderTarget* integratedBRDF = new D3D12RenderTarget(context->GetDevice(), Vec3i{ rtSize.x(), rtSize.y(), 1 }, DXGI_FORMAT_R32G32B32A32_FLOAT, "IrradianceMap");

	RENDER_EVENT(context, GenerateIntegratedBRDF);

	GraphicsPass pass(context);

	D3D12Geometry* geometry = mQuad;
	const Transformf& transform = Transformf(UniScalingf(1000.f));

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mPsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mShaderMacros.push_back(ShaderMacro{ "GENERATE_INTEGRATE_BRDF", "1" });

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { geometry->mInputDescs.data(), u32(geometry->mInputDescs.size()) };
	}

	pass.mRts[0] = integratedBRDF->GetRtv();
	pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(rtSize.x()), float(rtSize.y()));
	pass.mScissorRect = { 0, 0, rtSize.x(), rtSize.y() };
	pass.mStencilRef = 0;

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->mVbv);
	pass.mIbv = geometry->mIbv;
	pass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("RtSize", Vec4f{ f32(rtSize.x()), f32(rtSize.y()), 1.f / rtSize.x(), 1.f / rtSize.y() });

	pass.Draw();

	return integratedBRDF;
}

std::tuple<D3D12Backend::CommitedResource*, SRV*> EnvironmentMap::GeneratePrefilteredEnvironmentMap(GraphicsContext* context, IShaderResourceView* src, i32 resolution)
{
	const auto& originSize = src->GetResource()->GetSize();
	const auto& format = src->GetFormat();
	const i32 levelCount = std::log2(std::min<i32>(originSize.x(), originSize.y()));

	D3D12Backend::CommitedResource* result = D3D12Backend::CreateCommitedResourceTex2D(
		context->GetDevice(),
		Vec3i{ originSize.x(), originSize.y(), 1 },
		format,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		"FilteredEnvMap",
		levelCount);
	
	std::vector<RTV*> rtvs;
	std::vector<SRV*> srvs;

	for (i32 i = 0; i < levelCount; ++i)
	{
		rtvs.push_back(
			result->CreateRtv()
			.SetFormat(result->GetFormat())
			.SetViewDimension(D3D12_RTV_DIMENSION_TEXTURE2D)
			.SetMipSlice(i)
			.SetPlaneSlice(0)
			.BuildTex2D());

		srvs.push_back(
			result->CreateSrv()
			.SetFormat(result->GetFormat())
			.SetViewDimension(D3D12_SRV_DIMENSION_TEXTURE2D)
			.SetMostDetailedMip(i)
			.SetMipLevels(1)
			.SetPlaneSlice(0)
			.BuildTex2D());
	}

	SRV* fullSrv = result->CreateSrv()
		.SetFormat(result->GetFormat())
		.SetViewDimension(D3D12_SRV_DIMENSION_TEXTURE2D)
		.SetMostDetailedMip(0)
		.SetMipLevels(levelCount)
		.SetPlaneSlice(0)
		.BuildTex2D();

	RENDER_EVENT(context, FilterEnvironmentMap);

	Vec2f dstSize = Vec2f{ originSize.x(), originSize.y() };
	for (i32 i = 0; i < levelCount; ++i)
	{
		f32 roughness = f32(i) / (levelCount - 1);
		PrefilterEnvironmentMap(context, rtvs[i], src, Vec2i{ dstSize.x(), dstSize.y() }, roughness);
		dstSize = dstSize * 0.5f;
	}

	return std::make_tuple(result, fullSrv);
}

void EnvironmentMap::PrefilterEnvironmentMap(GraphicsContext* context, IRenderTargetView* target, IShaderResourceView* src, const Vec2i& targetSize, f32 roughness)
{
	static D3D12SamplerView* mPanoramicSkySampler = new D3D12SamplerView(context->GetDevice(), D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	static D3D12Geometry* mQuad = D3D12Geometry::GenerateQuad(context->GetDevice());

	GraphicsPass pass(context);

	D3D12Geometry* geometry = mQuad;
	const Transformf& transform = Transformf(UniScalingf(1000.f));

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mPsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mShaderMacros.push_back(ShaderMacro{ "PREFILTER_ENVIRONMENT_MAP", "1" });

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { geometry->mInputDescs.data(), u32(geometry->mInputDescs.size()) };
	}

	pass.mRts[0] = target;
	pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, float(targetSize.x()), float(targetSize.y()));
	pass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	pass.mStencilRef = 0;

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->mVbv);
	pass.mIbv = geometry->mIbv;
	pass.mIndexCount = geometry->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
	pass.AddCbVar("PrefilterInfo", Vec4f{ roughness, 0.f, 0.f, 0.f });

	pass.AddSrv("PanoramicSky", src);
	pass.AddSampler("PanoramicSkySampler", mPanoramicSkySampler);

	pass.Draw();
}
