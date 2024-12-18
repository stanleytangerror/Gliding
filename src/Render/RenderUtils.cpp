#include "Render/RenderPch.h"
#include "RenderUtils.h"
#include "Common/ModelProcess.h"
#include "Geometry.h"
#include "RenderMaterial.h"
#include "Texture.h"

namespace
{
	GI::SamplerDesc ToSamplerDesc(const ModelProcess::Sampler& sampler)
	{
		return GI::SamplerDesc()
			.SetFilter(GI::Filter::MIN_MAG_MIP_LINEAR)
			.SetAddress({ 
				sampler.mAddressMode[0],
				sampler.mAddressMode[1],
				sampler.mAddressMode[2]
				});
	}
}

void RenderUtils::CopyTexture(FrameGraph* frameGraph, 
	FrameGraphMutableResource& target,
	const Vec2f& targetOffset, const Vec2f& targetRect,
	const FrameGraphResource& source,
	const GI::SamplerDesc& sourceSampler, const char* sourcePixelUnary)
{
	static Geometry* geometry = Geometry::GenerateQuad()->CreateAndInitialResource(frameGraph);

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		SrvUsageFuture source;
		GI::SamplerDesc sourceSampler;
		RtvUsageFuture target;
		Vec3u targetSize;
		std::string sourcePixelUnary;
	};

	frameGraph->AddPass<PassData>("RenderUtils::CopyTexture",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.source = builder.ReadTex2DSrv(source);
			data.geoVertices = builder.ReadVbv(geometry->GetVb(), geometry->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(geometry->GetIb(), geometry->GetIbvDesc());
			data.sourceSampler = builder.Read(sourceSampler);
			data.targetSize = frameGraph->GetResourceDesc(target).GetSize();
			data.target = builder.WriteTex2DRtv(target);
			data.sourcePixelUnary = sourcePixelUnary;
		},
		[
			inputLayout = geometry->mVertexElementDescs,
			indexCount = geometry->mIndices.size(),
			targetOffset, targetRect
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.SetShader("CopyTexture", GI::ShaderMacro{ "SOURCE_PIXEL_UNARY", !data.sourcePixelUnary.empty() ? data.sourcePixelUnary : "color" });

			pass.SetupDepthStencil()
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.SetRtv(0, resources.Get(data.target));
			pass.mViewPort.SetTopLeftX(targetOffset.x()).SetTopLeftY(targetOffset.y()).SetWidth(targetRect.x()).SetHeight(targetRect.y());
			pass.mScissorRect = { 0, 0, i32(data.targetSize.x()), i32(data.targetSize.y()) };

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCb4f("RtSize", Vec4f{ targetRect.x(), targetRect.y(), 1.f / targetRect.x(), 1.f / targetRect.y() });
			pass.AddSrv("SourceTex", resources.Get(data.source));
			pass.AddSampler("SourceTexSampler", data.sourceSampler);

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void RenderUtils::CopyTexture(FrameGraph* frameGraph,
	FrameGraphMutableResource& target,
	const FrameGraphResource& source,
	const GI::SamplerDesc& sourceSampler)
{
	const auto& targetSize = frameGraph->GetResourceDesc(target).GetSize();
	CopyTexture(frameGraph, target, Vec2f::Zero(), Vec2f{ targetSize.x(), targetSize.y() }, source, sourceSampler);
}

void GaussianBlur1D(FrameGraph* frameGraph, FrameGraphMutableResource& target, const FrameGraphResource& source, i32 kernelSizeInPixel, const GI::SamplerDesc& sampler, Geometry* geometry, bool isHorizontal)
{
	auto NormalDistPdf = [](f32 x, f32 stdDev) { return exp(-0.5f * (x * x / stdDev / stdDev) / stdDev) / Math::Sqrt(2.f * Math::Pi<f32>()); };

	const auto& size = frameGraph->GetResourceDesc(source).GetSize();
	const auto& weight4fSize = (kernelSizeInPixel + 1 + 3) / 4;

	struct PassData
	{
		VbvUsageFuture geoVertices;
		IbvUsageFuture geoIndices;
		SrvUsageFuture source;
		GI::SamplerDesc sampler;
		RtvUsageFuture target;
		Vec3u size;
	};

	frameGraph->AddPass<PassData>("GaussianBlur1D",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.source = builder.ReadTex2DSrv(source);
			data.geoVertices = builder.ReadVbv(geometry->GetVb(), geometry->GetVbvDesc());
			data.geoIndices = builder.ReadIbv(geometry->GetIb(), geometry->GetIbvDesc());
			data.sampler = builder.Read(sampler);
			data.size = size;
			data.target = builder.WriteTex2DRtv(target);
		},
		[
			inputLayout = geometry->mVertexElementDescs,
			indexCount = geometry->mIndices.size(),
			isHorizontal, weight4fSize, NormalDistPdf, kernelSizeInPixel
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			RENDER_EVENT(infra, GaussianBlur1D);

			GI::GraphicsPass pass;

			pass.SetShader("GaussianBlur",
				GI::ShaderMacro{ "WEIGHT_SIZE", Utils::FormatString("%d", weight4fSize) },
				GI::ShaderMacro{ isHorizontal ? "HORIZONTAL" : "VERTICAL", "1" });

			pass.SetupDepthStencil()
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.SetRtv(0, resources.Get(data.target));
			pass.SetViewPortAndScissorRectToFullRt();

			pass.SetGeometry(
				resources.Get(data.geoVertices), 0, inputLayout,
				resources.Get(data.geoIndices), 0, indexCount);

			pass.AddCb4f("BlurTargetSize", Vec4f{ f32(data.size.x()), f32(data.size.y()), 1.f / data.size.x(), 1.f / data.size.y() });
			pass.AddSrv("SourceTex", resources.Get(data.source));
			pass.AddSampler("SourceTexSampler", data.sampler);

			std::vector<f32> weights;
			f32 totalWeight = 0.f;
			for (i32 i = 0; i < weight4fSize * 4; ++i)
			{
				f32 w = NormalDistPdf(i, kernelSizeInPixel * 0.5f);
				weights.push_back(w);
				totalWeight += (i == 0) ? w : w * 2;
			}
			for (auto& w : weights)
			{
				w /= totalWeight;
			}
			pass.AddCbNf("Weights", weights);

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void RenderUtils::GaussianBlur(FrameGraph* frameGraph, 
	FrameGraphMutableResource& target, const FrameGraphResource& source, i32 kernelSizeInPixel)
{
	static GI::SamplerDesc sampler;
	static Geometry* geometry = Geometry::GenerateQuad()->CreateAndInitialResource(frameGraph);
	
	{
		sampler
			.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
			.SetAddressXYZ(GI::TextureAddressMode::WRAP);
	}

	auto sourceDesc = frameGraph->GetResourceDesc(source);

	const auto desc = GI::MemoryResourceDesc::RenderTarget2D(Vec2u{ sourceDesc.GetWidth(), sourceDesc.GetHeight() }, sourceDesc.GetFormat(), GI::ResourceFlag::ALLOW_RENDER_TARGET, "GaussianBlurIntermediateRt");
	auto interRtFg = frameGraph->CreateTransient(desc);

	GaussianBlur1D(frameGraph, interRtFg, source, kernelSizeInPixel, sampler, geometry, true);
	GaussianBlur1D(frameGraph, target, interRtFg, kernelSizeInPixel, sampler, geometry, false);
}

TransformNode<std::pair<
	std::unique_ptr<Geometry>,
	std::shared_ptr<RenderMaterial>>>* RenderUtils::GenerateMaterialProbes(FrameGraph* frameGraph)
{
	auto result = new TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>;


	static Geometry* geo = Geometry::GenerateSphere(40)->CreateAndInitialResource(frameGraph);
	static GI::SamplerDesc sampler;

	{
		sampler
			.SetFilter(GI::Filter::MIN_MAG_MIP_LINEAR)
			.SetAddressXYZ(GI::TextureAddressMode::WRAP);
	}

	auto genMesh = [&](f32 roughness, f32 metallic, const Vec3f& pos)
	{
		RenderMaterial* material = new RenderMaterial;
		material->mBaseColorChannel.mColor = Vec4f::Ones() * 0.5f;
		material->mMetallicRoughnessChannel.mMetallicFactor = metallic;
		material->mMetallicRoughnessChannel.mRoughnessFactor = roughness;
		material->mNormalChannel.mNormalConstant = Vec3f{ 0.f, 0.f, 1.f };

		result->PushChild(std::pair<
			std::unique_ptr<Geometry>,
			std::shared_ptr<RenderMaterial>>{ geo, material }, Transformf(Translationf(pos)));
	};

	for (f32 roughness = 0.f; roughness <= 1.05f; roughness += 0.25f)
	{
		for (f32 metallic = 0.f; metallic <= 1.05f; metallic += 0.25f)
		{
			genMesh(roughness, metallic, 10.f * Vec3f{ roughness - 0.5f, 0.f, metallic - 0.5f });
		}
	}

	return result;
}

TransformNode<std::pair<
	std::shared_ptr<Geometry>,
	std::shared_ptr<RenderMaterial>>>* RenderUtils::FromModelData(FrameGraph* frameGraph, const ModelProcess::Model& model)
{
	auto result = new TransformNode<std::pair<
		std::shared_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>;

	std::map<Guid, std::pair<FileTexture*, GI::SamplerDesc>> textures;
	for (const auto& tex : model.Textures)
	{
		textures[tex.mId] = {
			new FileTexture(frameGraph, tex.mPath.c_str(), Utils::LoadFileContent(tex.mPath.c_str())),
			ToSamplerDesc(tex.mSampler)
		};
	}

	std::map<Guid, std::shared_ptr<RenderMaterial>> materials;
	for (const auto& mat : model.Materials)
	{
		materials[mat.Id] = std::shared_ptr<RenderMaterial>(RenderMaterial::GenerateRenderMaterialFromMaterialData(frameGraph, mat, textures));
	}

	std::map<Guid, std::pair<std::shared_ptr<Geometry>, std::shared_ptr<RenderMaterial>>> meshes;
	for (const auto& mesh : model.Meshes)
	{
		Geometry* geo = GenerateGeometryFromMeshData(mesh);
		geo->CreateAndInitialResource(frameGraph);

		meshes[mesh.Id] =
		{
			std::shared_ptr<Geometry>(geo),
			materials[mesh.MaterialId]
		};
	}

	for (const auto& inst : model.MeshInstances)
	{
		result->PushChild(meshes[inst.MeshId], Transformf(inst.LocalTransform));
	}

	return result;
}

Geometry* RenderUtils::GenerateGeometryFromMeshData(const ModelProcess::Mesh& mesh)
{
	static const char* names[] =
	{
		"POSITION",
		"NORMAL",
		"TANGENT",
		"BINORMAL",
		"TEXCOORD",
		"COLOR"
	};

	static const GI::Format::Enum formats[] =
	{
		GI::Format::FORMAT_R32_FLOAT,
		GI::Format::FORMAT_R32G32_FLOAT,
		GI::Format::FORMAT_R32G32B32_FLOAT,
		GI::Format::FORMAT_R32G32B32A32_FLOAT,
	};

	auto vertexStride = std::accumulate(
		mesh.VertexAttributeMetas.begin(),
		mesh.VertexAttributeMetas.end(),
		0,
		[](i32 v, const auto& meta) { return v + meta.mSizeInBytes; }
	);

	std::vector<GI::InputElementDesc> inputDesc;
	for (const auto& meta : mesh.VertexAttributeMetas)
	{
		inputDesc.push_back(GI::InputElementDesc()
			.SetSemanticName(names[meta.mSemantic])
			.SetSemanticIndex(meta.mSemanticIndex)
			.SetFormat(formats[meta.mSizeInBytes / sizeof(f32)])
			.SetInputSlot(0)
			.SetAlignedByteOffset(meta.mOffsetInBytes));
	}

	return Geometry::GenerateGeometry(mesh.Vertices, vertexStride, mesh.Indices, inputDesc);
}