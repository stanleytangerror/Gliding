#include "Render/RenderPch.h"
#include "EnvironmentMap.h"
#include "Geometry.h"

FrameGraphResource EnvironmentMap::GenerateIrradianceMap(
	const Geometry* quad, 
	FrameGraph* frameGraph, 
	const FrameGraphResource& sky, i32 resolution, i32 semiSphereBusbarSampleCount)
{
	static GI::SamplerDesc mPanoramicSkySampler;

	{
		mPanoramicSkySampler
			.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
			.SetAddressXYZ(GI::TextureAddressMode::WRAP);
	}

	const Vec2u& rtSize = { u32(resolution * 2), u32(resolution) };
	auto format = GI::Format::FORMAT_R32G32B32A32_FLOAT;

	auto irradianceMap = frameGraph->CreatePermanent(
		GI::MemoryResourceDesc::RenderTarget2D(
			rtSize,
			format,
			GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS,
			"IrradianceMap"));

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		SrvUsageFuture sky;
		GI::SamplerDesc panoramicSkySampler;
		RtvUsageFuture rtv;
	};

	frameGraph->AddPass<PassData>("GenerateIrradianceMap",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.sky = builder.ReadTex2DSrv(sky);
			data.geoVertices = builder.ReadVbv(quad->GetVb(), quad->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(quad->GetIb(), quad->GetIbvDesc());
			data.panoramicSkySampler = builder.Read(mPanoramicSkySampler);

			data.rtv = builder.WriteTex2DRtv(irradianceMap);
		},
		[
			inputLayout = quad->mVertexElementDescs,
			indexCount = quad->mIndices.size(),
			rtSize, semiSphereBusbarSampleCount
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, GenerateIrradianceMap);

			GI::GraphicsPass pass;

			const Transformf& transform = Transformf(UniScalingf(1000.f));

			pass.SetShader("EnvironmentMap", GI::ShaderMacro{ "GENERATE_IRRADIANCE_MAP", "1" });

			pass.SetupDepthStencil()
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.SetRtv(0, resources.Get(data.rtv));
			pass.SetViewPortAndScissorRectToFullRt();
			
			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCb4f("RtSize", Vec4f{ f32(rtSize.x()), f32(rtSize.y()), 1.f / rtSize.x(), 1.f / rtSize.y() });

			const f32 deltaRad = Math::Pi<f32>() / 2.f / semiSphereBusbarSampleCount;
			const f32 sampleCount = semiSphereBusbarSampleCount * semiSphereBusbarSampleCount * 4.f;
			pass.AddCb4f("SemiSphereSampleInfo", Vec4f{ deltaRad, sampleCount, 1.f / sampleCount, 0.f });

			pass.AddSrv("PanoramicSky", resources.Get(data.sky));
			pass.AddSampler("PanoramicSkySampler", data.panoramicSkySampler);

			infra->GetRecorder()->AddGraphicsPass(pass);
		});

	return irradianceMap;
}

FrameGraphResource EnvironmentMap::GenerateIntegratedBRDF(
	const Geometry* quad, FrameGraph* frameGraph, i32 resolution)
{
	static GI::SamplerDesc mPanoramicSkySampler;
	{
		mPanoramicSkySampler
			.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
			.SetAddressXYZ(GI::TextureAddressMode::WRAP);
	}

	const Vec2u& rtSize = { u32(resolution), u32(resolution) };
	auto format = GI::Format::FORMAT_R32G32B32A32_FLOAT;

	auto integrateBrdf = frameGraph->CreatePermanent(GI::MemoryResourceDesc::RenderTarget2D(
		rtSize, 
		format, 
		GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS, 
		"IntegratedBRDF"));

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		RtvUsageFuture rtv;
	};

	frameGraph->AddPass<PassData>("GenerateIntegratedBRDF",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.ReadVbv(quad->GetVb(), quad->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(quad->GetIb(), quad->GetIbvDesc());
			data.rtv = builder.WriteTex2DRtv(integrateBrdf);
		},
		[
			inputLayout = quad->mVertexElementDescs,
				indexCount = quad->mIndices.size(),
				rtSize
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, GenerateIntegratedBRDF);

			GI::GraphicsPass pass;

			const Transformf& transform = Transformf(UniScalingf(1000.f));

			pass.SetShader("EnvironmentMap", GI::ShaderMacro{ "GENERATE_INTEGRATE_BRDF", "1" });

			pass.SetupDepthStencil()
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.SetRtv(0, resources.Get(data.rtv));
			pass.SetViewPortAndScissorRectToFullRt();

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCb4f("RtSize", Vec4f{ f32(rtSize.x()), f32(rtSize.y()), 1.f / rtSize.x(), 1.f / rtSize.y() });

			infra->GetRecorder()->AddGraphicsPass(pass);
		});

	return integrateBrdf;
}

FrameGraphResource EnvironmentMap::GeneratePrefilteredEnvironmentMap(
	const Geometry* quad,
	FrameGraph* frameGraph,
	const FrameGraphResource& src, i32 resolution)
{
	const auto& srcResDesc = frameGraph->GetResourceDesc(src);
	const auto& originSize = srcResDesc.GetSize();
	const auto& format = srcResDesc.GetFormat();
	const i32 levelCount = std::log2(std::min<i32>(originSize.x(), originSize.y()));

	auto filteredMapDesc = GI::MemoryResourceDesc::RenderTarget2D(
		{ originSize.x(), originSize.y() },
		format,
		GI::ResourceFlag::ALLOW_RENDER_TARGET | GI::ResourceFlag::ALLOW_UNORDERED_ACCESS,
		"FilteredEnvMap")
		.SetMipLevels(levelCount);

	auto filteredMap = frameGraph->CreatePermanent(filteredMapDesc);

	std::vector<GI::RtvDesc> rtvs;

	for (i32 i = 0; i < levelCount; ++i)
	{
		rtvs.push_back(GI::RtvDesc()
				.SetFormat(filteredMapDesc.GetFormat())
				.SetViewDimension(GI::RtvDimension::TEXTURE2D)
				.SetTexture2D_MipSlice(i)
				.SetTexture2D_PlaneSlice(0));
	}

	//RENDER_EVENT(infra, FilterEnvironmentMap);

	Vec2f dstSize = Vec2f{ originSize.x(), originSize.y() };
	for (i32 i = 0; i < levelCount; ++i)
	{
		f32 roughness = f32(i) / (levelCount - 1);
		PrefilterEnvironmentMap(quad, frameGraph, filteredMap, rtvs[i], src, Vec2i{ dstSize.x(), dstSize.y() }, roughness);
		dstSize = dstSize * 0.5f;
	}

	return filteredMap;
}

void EnvironmentMap::PrefilterEnvironmentMap(
	const Geometry* quad,
	FrameGraph* frameGraph, 
	FrameGraphMutableResource& targetResource, GI::RtvDesc& targetDesc, 
	const FrameGraphResource& src,
	const Vec2i& targetSize, f32 roughness)
{
	static GI::SamplerDesc mPanoramicSkySampler;
	{
		mPanoramicSkySampler
			.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
			.SetAddressXYZ(GI::TextureAddressMode::WRAP);
	}

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		GI::SamplerDesc sampler;
		SrvUsageFuture src;
		RtvUsageFuture target;
	};

	frameGraph->AddPass<PassData>("FilterEnvironmentMap",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.geoVertices = builder.ReadVbv(quad->GetVb(), quad->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(quad->GetIb(), quad->GetIbvDesc());
			data.sampler = mPanoramicSkySampler;
			data.src = builder.ReadTex2DSrv(src);

			data.target = builder.Write(targetResource, targetDesc);
		},
		[
			inputLayout = quad->mVertexElementDescs,
				indexCount = quad->mIndices.size(),
				targetSize, roughness
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, FilterEnvironmentMap);
			
			GI::GraphicsPass pass;

			const Transformf& transform = Transformf(UniScalingf(1000.f));

			pass.SetShader("EnvironmentMap", GI::ShaderMacro{ "PREFILTER_ENVIRONMENT_MAP", "1" });

			pass.SetupDepthStencil()
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.SetRtv(0, resources.Get(data.target));
			pass.SetViewPortAndScissorRectToFullRt();

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCb4f("RtSize", Vec4f{ f32(targetSize.x()), f32(targetSize.y()), 1.f / targetSize.x(), 1.f / targetSize.y() });
			pass.AddCb4f("PrefilterInfo", Vec4f{ roughness, 0.f, 0.f, 0.f });

			pass.AddSrv("PanoramicSky", resources.Get(data.src));
			pass.AddSampler("PanoramicSkySampler", data.sampler);

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}
