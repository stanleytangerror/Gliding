#include "RenderPch.h"
#include "EnvironmentMap.h"
#include "Geometry.h"

std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, GI::SrvDesc> EnvironmentMap::GenerateIrradianceMap(GI::IGraphicsInfra* infra, const GI::SrvDesc& sky, i32 resolution, i32 semiSphereBusbarSampleCount)
{
	static GI::SamplerDesc mPanoramicSkySampler;
	static Geometry* mQuad = Geometry::GenerateQuad();

	if (!mQuad->IsGraphicsResourceReady())
	{
		mPanoramicSkySampler
			.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
			.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP });

		mQuad->CreateAndInitialResource(infra);
	}

	const Vec2i& rtSize = { resolution * 2, resolution };
	auto format = GI::Format::FORMAT_R32G32B32A32_FLOAT;
	auto irradianceMap = infra->CreateMemoryResource(
		GI::MemoryResourceDesc()
		.SetAlignment(0)
		.SetDimension(GI::ResourceDimension::TEXTURE2D)
		.SetWidth(rtSize.x())
		.SetHeight(rtSize.y())
		.SetDepthOrArraySize(1)
		.SetMipLevels(1)
		.SetFormat(format)
		.SetLayout(GI::TextureLayout::LAYOUT_UNKNOWN)
		.SetFlags(GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS)
		.SetInitState(GI::ResourceState::STATE_RENDER_TARGET)
		.SetName("IrradianceMap")
		.SetHeapType(GI::HeapType::DEFAULT));
		
	const auto& rtv = GI::RtvDesc()
		.SetResource(irradianceMap.get())
		.SetFormat(format)
		.SetViewDimension(GI::RtvDimension::TEXTURE2D);

	const auto& srv = GI::SrvDesc()
		.SetResource(irradianceMap.get())
		.SetFormat(format)
		.SetViewDimension(GI::SrvDimension::TEXTURE2D)
		.SetTexture2D_MipLevels(1);

	RENDER_EVENT(context, GenerateIrradianceMap);

	GI::GraphicsPass pass;

	Geometry* geometry = mQuad;
	const Transformf& transform = Transformf(UniScalingf(1000.f));

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mPsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mShaderMacros.push_back(GI::ShaderMacro{ "GENERATE_IRRADIANCE_MAP", "1" });

	
	pass.mDepthStencilDesc
		.SetDepthEnable(false)
		.SetStencilEnable(false);

	pass.mInputLayout = geometry->mVertexElementDescs;

	pass.mRtvs[0] = rtv;
	pass.mViewPort.SetWidth(rtSize.x()).SetHeight(rtSize.y());
	pass.mScissorRect = { 0, 0, rtSize.x(), rtSize.y() };
	pass.mStencilRef = 0;

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->GetVbvDesc());
	pass.mIbv = geometry->GetIbvDesc();
	pass.mIndexCount = geometry->mIndices.size();

	pass.AddCbVar("RtSize", Vec4f{ f32(rtSize.x()), f32(rtSize.y()), 1.f / rtSize.x(), 1.f / rtSize.y() });

	const f32 deltaRad = Math::Pi<f32>() / 2.f / semiSphereBusbarSampleCount;
	const f32 sampleCount = semiSphereBusbarSampleCount * semiSphereBusbarSampleCount * 4.f;
	pass.AddCbVar("SemiSphereSampleInfo", Vec4f{ deltaRad, sampleCount, 1.f / sampleCount, 0.f });

	pass.AddSrv("PanoramicSky", sky);
	pass.AddSampler("PanoramicSkySampler", mPanoramicSkySampler);

	infra->GetRecorder()->AddGraphicsPass(pass);

	return std::make_tuple(std::move(irradianceMap), srv);
}

std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, GI::SrvDesc> EnvironmentMap::GenerateIntegratedBRDF(GI::IGraphicsInfra* infra, i32 resolution)
{
	static GI::SamplerDesc mPanoramicSkySampler;
	static Geometry* mQuad = Geometry::GenerateQuad();

	if (!mQuad->IsGraphicsResourceReady())
	{
		mPanoramicSkySampler
			.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
			.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP });

		mQuad->CreateAndInitialResource(infra);
	}

	const Vec2i& rtSize = { resolution, resolution };
	auto format = GI::Format::FORMAT_R32G32B32A32_FLOAT;

	auto integratedBRDF = infra->CreateMemoryResource(
		GI::MemoryResourceDesc()
		.SetAlignment(0)
		.SetDimension(GI::ResourceDimension::TEXTURE2D)
		.SetWidth(rtSize.x())
		.SetHeight(rtSize.y())
		.SetDepthOrArraySize(1)
		.SetMipLevels(1)
		.SetFormat(format)
		.SetLayout(GI::TextureLayout::LAYOUT_UNKNOWN)
		.SetFlags(GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS)
		.SetInitState(GI::ResourceState::STATE_RENDER_TARGET)
		.SetName("IntegratedBRDF")
		.SetHeapType(GI::HeapType::DEFAULT));

	const auto& rtv = GI::RtvDesc()
		.SetResource(integratedBRDF.get())
		.SetFormat(format)
		.SetViewDimension(GI::RtvDimension::TEXTURE2D);

	const auto& srv = GI::SrvDesc()
		.SetResource(integratedBRDF.get())
		.SetFormat(format)
		.SetViewDimension(GI::SrvDimension::TEXTURE2D)
		.SetTexture2D_MipLevels(1);

	RENDER_EVENT(context, GenerateIntegratedBRDF);

	GI::GraphicsPass pass;

	Geometry* geometry = mQuad;
	const Transformf& transform = Transformf(UniScalingf(1000.f));

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mPsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mShaderMacros.push_back(GI::ShaderMacro{ "GENERATE_INTEGRATE_BRDF", "1" });

	
	pass.mDepthStencilDesc
		.SetDepthEnable(false)
		.SetStencilEnable(false);

	pass.mInputLayout = geometry->mVertexElementDescs;

	pass.mRtvs[0] = rtv;
	pass.mViewPort.SetWidth(rtSize.x()).SetHeight(rtSize.y());
	pass.mScissorRect = { 0, 0, rtSize.x(), rtSize.y() };
	pass.mStencilRef = 0;

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->GetVbvDesc());
	pass.mIbv = geometry->GetIbvDesc();
	pass.mIndexCount = geometry->mIndices.size();

	pass.AddCbVar("RtSize", Vec4f{ f32(rtSize.x()), f32(rtSize.y()), 1.f / rtSize.x(), 1.f / rtSize.y() });

	infra->GetRecorder()->AddGraphicsPass(pass);

	return std::make_tuple(std::move(integratedBRDF), srv);
}

std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, GI::SrvDesc> EnvironmentMap::GeneratePrefilteredEnvironmentMap
(GI::IGraphicsInfra* infra, const GI::SrvDesc& src, i32 resolution)
{
	const auto& originSize = src.GetResource()->GetDimSize();
	const auto& format = src.GetResource()->GetFormat();
	const i32 levelCount = std::log2(std::min<i32>(originSize.x(), originSize.y()));

	auto result = infra->CreateMemoryResource(
		GI::MemoryResourceDesc()
		.SetAlignment(0)
		.SetDimension(GI::ResourceDimension::TEXTURE2D)
		.SetWidth(originSize.x())
		.SetHeight(originSize.y())
		.SetDepthOrArraySize(1)
		.SetMipLevels(levelCount)
		.SetFormat(format)
		.SetLayout(GI::TextureLayout::LAYOUT_UNKNOWN)
		.SetFlags(GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS)
		.SetInitState(GI::ResourceState::STATE_RENDER_TARGET)
		.SetName("FilteredEnvMap")
		.SetHeapType(GI::HeapType::DEFAULT));

	std::vector<GI::RtvDesc> rtvs;
	std::vector<GI::SrvDesc> srvs;

	for (i32 i = 0; i < levelCount; ++i)
	{
		rtvs.push_back(
			GI::RtvDesc()
			.SetFormat(result->GetFormat())
			.SetViewDimension(GI::RtvDimension::TEXTURE2D)
			.SetTexture2D_MipSlice(i)
			.SetTexture2D_PlaneSlice(0));

		srvs.push_back(
			GI::SrvDesc()
			.SetFormat(result->GetFormat())
			.SetViewDimension(GI::SrvDimension::TEXTURE2D)
			.SetTexture2D_MostDetailedMip(i)
			.SetTexture2D_MipLevels(1)
			.SetTexture2D_PlaneSlice(0));
	}

	GI::SrvDesc fullSrv;
	fullSrv
		.SetFormat(result->GetFormat())
		.SetViewDimension(GI::SrvDimension::TEXTURE2D)
		.SetTexture2D_MostDetailedMip(0)
		.SetTexture2D_MipLevels(levelCount)
		.SetTexture2D_PlaneSlice(0);

	RENDER_EVENT(context, FilterEnvironmentMap);

	Vec2f dstSize = Vec2f{ originSize.x(), originSize.y() };
	for (i32 i = 0; i < levelCount; ++i)
	{
		f32 roughness = f32(i) / (levelCount - 1);
		PrefilterEnvironmentMap(infra, rtvs[i], src, Vec2i{ dstSize.x(), dstSize.y() }, roughness);
		dstSize = dstSize * 0.5f;
	}

	return std::make_tuple(std::move(result), fullSrv);
}

void EnvironmentMap::PrefilterEnvironmentMap
(GI::IGraphicsInfra* infra, const GI::RtvDesc& target, const GI::SrvDesc& src, const Vec2i& targetSize, f32 roughness)
{
	static GI::SamplerDesc mPanoramicSkySampler;
	static Geometry* mQuad = Geometry::GenerateQuad();

	if (!mQuad->IsGraphicsResourceReady())
	{
		mPanoramicSkySampler
			.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
			.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP });

		mQuad->CreateAndInitialResource(infra);
	}

	GI::GraphicsPass pass;

	Geometry* geometry = mQuad;
	const Transformf& transform = Transformf(UniScalingf(1000.f));

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mPsFile = "res/Shader/EnvironmentMap.hlsl";
	pass.mShaderMacros.push_back(GI::ShaderMacro{ "PREFILTER_ENVIRONMENT_MAP", "1" });

	
	pass.mDepthStencilDesc
		.SetDepthEnable(false)
		.SetStencilEnable(false);

	pass.mInputLayout = geometry->mVertexElementDescs;

	pass.mRtvs[0] = target;
	pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
	pass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };
	pass.mStencilRef = 0;

	pass.mVbvs.clear();
	pass.mVbvs.push_back(geometry->GetVbvDesc());
	pass.mIbv = geometry->GetIbvDesc();
	pass.mIndexCount = geometry->mIndices.size();

	pass.AddCbVar("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
	pass.AddCbVar("PrefilterInfo", Vec4f{ roughness, 0.f, 0.f, 0.f });

	pass.AddSrv("PanoramicSky", src);
	pass.AddSampler("PanoramicSkySampler", mPanoramicSkySampler);

	infra->GetRecorder()->AddGraphicsPass(pass);
}
