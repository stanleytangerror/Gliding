#include "RenderPch.h"
#include "RenderUtils.h"
#include "Geometry.h"
#include "RenderTarget.h"
#include "World/Scene.h"
#include "RenderMaterial.h"
#include "Texture.h"

namespace
{
	GI::SamplerDesc ToSamplerDesc(const TextureSamplerType& type)
	{
		auto mapType = [](SamplerAddrMode mode)
		{
			switch (mode)
			{
			case TextureSamplerType_Clamp: return GI::TextureAddressMode::CLAMP;
			case TextureSamplerType_Mirror: return GI::TextureAddressMode::MIRROR;
			case TextureSamplerType_Wrap: return GI::TextureAddressMode::WRAP;
			case TextureSamplerType_Decal:
			default:  Assert(false); return GI::TextureAddressMode::WRAP;
			}
		};

		return GI::SamplerDesc()
			.SetFilter(GI::Filter::MIN_MAG_MIP_LINEAR)
			.SetAddress({ mapType(type[0]),
					mapType(type[1]),
					mapType(type[2]) });
	}
}

void RenderUtils::CopyTexture(FrameGraph* frameGraph, GI::IGraphicsInfra* infra,
	const RtvUsageFuture& target,
	const Vec2f& targetOffset, const Vec2f& targetRect,
	const SrvUsageFuture& source,
	const GI::SamplerDesc& sourceSampler, const char* sourcePixelUnary)
{
	static Geometry* quad = Geometry::GenerateQuad();
	if (!quad->IsGraphicsResourceReady()) 
	{ 
		quad->CreateAndInitialResource(infra); 
	}

	struct PassData
	{
		GI::VbvUsage geoVertices;
		GI::IbvUsage geoIndices;
		SrvUsageFuture source;
		GI::SamplerDesc sourceSampler;
		RtvUsageFuture target;
		Vec3u targetSize;
	};

	frameGraph->AddPass<PassData>("RenderUtils::CopyTexture",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.source = builder.Read(source);
			data.geoVertices = builder.Read(quad->GetVbvDesc());
			data.geoIndices = builder.Read(quad->GetIbvDesc());
			data.sourceSampler = builder.Read(sourceSampler);
			data.targetSize = frameGraph->GetResourceDesc(target.resource).GetSize();
			data.target = builder.Write(target);
		},
		[
			inputLayout = quad->mVertexElementDescs,
			indexCount = quad->mIndices.size(),
			sourcePixelUnary, targetOffset, targetRect
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
			pass.mRootSignatureDesc.mEntry = "GraphicsRS";
			pass.mVsFile = "res/Shader/CopyTexture.hlsl";
			pass.mPsFile = "res/Shader/CopyTexture.hlsl";
			pass.mShaderMacros.push_back(GI::ShaderMacro{ "SOURCE_PIXEL_UNARY", sourcePixelUnary ? sourcePixelUnary : "color" });

			pass.mDepthStencilDesc
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.mInputLayout = inputLayout;

			pass.SetRtv(0, resources.Get(data.target));
			pass.mViewPort.SetTopLeftX(targetOffset.x()).SetTopLeftY(targetOffset.y()).SetWidth(targetRect.x()).SetHeight(targetRect.y());
			pass.mScissorRect = { 0, 0, i32(data.targetSize.x()), i32(data.targetSize.y()) };

			pass.PushVbv(data.geoVertices);
			pass.SetIbv(data.geoIndices);
			pass.mIndexCount = indexCount;

			pass.AddCbVar("RtSize", Vec4f{ targetRect.x(), targetRect.y(), 1.f / targetRect.x(), 1.f / targetRect.y() });
			pass.AddSrv("SourceTex", resources.Get(data.source));
			pass.AddSampler("SourceTexSampler", data.sourceSampler);

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void RenderUtils::CopyTexture(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, 
	const RtvUsageFuture& target,
	const SrvUsageFuture& source,
	const GI::SamplerDesc& sourceSampler)
{
	const auto& targetSize = frameGraph->GetResourceDesc(target.resource).GetSize();
	CopyTexture(frameGraph, infra, target, Vec2f::Zero(), Vec2f{ targetSize.x(), targetSize.y() }, source, sourceSampler);
}

void GaussianBlur1D(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, const RtvUsageFuture& target, const SrvUsageFuture& source, i32 kernelSizeInPixel, const GI::SamplerDesc& sampler, Geometry* quad, bool isHorizontal)
{
	auto NormalDistPdf = [](f32 x, f32 stdDev) { return exp(-0.5f * (x * x / stdDev / stdDev) / stdDev) / Math::Sqrt(2.f * Math::Pi<f32>()); };

	const auto& size = frameGraph->GetResourceDesc(source.resource).GetSize();
	const auto& weight4fSize = (kernelSizeInPixel + 1 + 3) / 4;

	struct PassData
	{
		GI::VbvUsage geoVertices;
		GI::IbvUsage geoIndices;
		SrvUsageFuture source;
		GI::SamplerDesc sampler;
		RtvUsageFuture target;
		Vec3u size;
	};

	frameGraph->AddPass<PassData>("GaussianBlur1D",
		[&]
		(RenderPassBuilder& builder, PassData& data)
		{
			data.source = builder.Read(source);
			data.geoVertices = builder.Read(quad->GetVbvDesc());
			data.geoIndices = builder.Read(quad->GetIbvDesc());
			data.sampler = builder.Read(sampler);
			data.size = size;
			data.target = builder.Write(target);
		},
		[
			inputLayout = quad->mVertexElementDescs,
			indexCount = quad->mIndices.size(),
			isHorizontal, weight4fSize, NormalDistPdf, kernelSizeInPixel
		]
		(const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			GI::GraphicsPass pass;

			pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
			pass.mRootSignatureDesc.mEntry = "GraphicsRS";
			pass.mVsFile = "res/Shader/GaussianBlur.hlsl";
			pass.mPsFile = "res/Shader/GaussianBlur.hlsl";
			pass.mShaderMacros.push_back(GI::ShaderMacro{ "WEIGHT_SIZE", Utils::FormatString("%d", weight4fSize) });
			pass.mShaderMacros.push_back(GI::ShaderMacro{ isHorizontal ? "HORIZONTAL" : "VERTICAL", "1" });

			pass.mDepthStencilDesc
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.mInputLayout = inputLayout;

			pass.SetRtv(0, resources.Get(data.target));
			pass.mViewPort.SetWidth(data.size.x()).SetHeight(data.size.y());
			pass.mScissorRect = { 0, 0, i32(data.size.x()), i32(data.size.y()) };

			pass.PushVbv(data.geoVertices);
			pass.SetIbv(data.geoIndices);
			pass.mIndexCount = indexCount;

			pass.AddCbVar("BlurTargetSize", Vec4f{ f32(data.size.x()), f32(data.size.y()), 1.f / data.size.x(), 1.f / data.size.y() });
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
			pass.AddCbVar("Weights", weights);

			infra->GetRecorder()->AddGraphicsPass(pass);
		});
}

void RenderUtils::GaussianBlur(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, 
	const RtvUsageFuture& target, const SrvUsageFuture& source, i32 kernelSizeInPixel)
{
	static GI::SamplerDesc sampler;
	static Geometry* quad = Geometry::GenerateQuad();
	
	if (!quad->IsGraphicsResourceReady())
	{
		sampler
			.SetFilter(GI::Filter::MIN_MAG_LINEAR_MIP_POINT)
			.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP });

		quad->CreateAndInitialResource(infra);
	}

	auto sourceDesc = frameGraph->GetResourceDesc(source.resource);

	const auto desc = GI::MemoryResourceDesc::RenderTarget2D(Vec2u{ sourceDesc.GetWidth(), sourceDesc.GetHeight() }, sourceDesc.GetFormat(), false, "GaussianBlurIntermediateRt");
	auto interRtFg = frameGraph->Create(desc);

	RENDER_EVENT(infra, GaussianBlur);
	GaussianBlur1D(frameGraph, infra, { interRtFg, GI::MemoryResourceDesc::AsTexture2DRtv(desc) }, source, kernelSizeInPixel, sampler, quad, true);
	GaussianBlur1D(frameGraph, infra, target, { interRtFg, GI::MemoryResourceDesc::AsTexture2DSrv(desc) }, kernelSizeInPixel, sampler, quad, false);
}

TransformNode<std::pair<
	std::unique_ptr<Geometry>,
	std::shared_ptr<RenderMaterial>>>*
RenderUtils::FromSceneRawData(GI::IGraphicsInfra* infra, SceneRawData* sceneRawData)
{
	auto result = new TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>;

	std::map<std::string, FileTexture*> textures;
	for (const auto& [texPath, texRawData] : sceneRawData->mTextures)
	{
		if (texRawData)
		{
			textures[texPath] = new FileTexture(infra, texPath.c_str(), texRawData->mRawData);
		}
	}
	std::map<TextureSamplerType, GI::SamplerDesc> samplers;
	for (const TextureSamplerType& samplerType : sceneRawData->mSamplers)
	{
		samplers[samplerType] = ToSamplerDesc(samplerType);
	}

	std::vector<std::shared_ptr<RenderMaterial>> materials;
	for (const auto& mat : sceneRawData->mMaterials)
	{
		materials.emplace_back(RenderMaterial::GenerateRenderMaterialFromRawData(mat, sceneRawData, textures, samplers));
	}
	for (MeshRawData* mesh : sceneRawData->mMeshes)
	{
		Geometry* geo = GenerateGeometryFromMeshRawData(mesh);
		const auto& mat = materials[mesh->mMaterialIndex];
		const Transformf& trans = mesh->mTransform;

		result->PushChild({
			std::unique_ptr<Geometry>(geo),
			mat },
			trans);
	}

	return result;
}

TransformNode<std::pair<
	std::unique_ptr<Geometry>,
	std::shared_ptr<RenderMaterial>>>* RenderUtils::GenerateMaterialProbes(GI::IGraphicsInfra* infra)
{
	auto result = new TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>;


	static Geometry* geo = Geometry::GenerateSphere(40);
	static GI::SamplerDesc sampler;

	if (!geo->IsGraphicsResourceReady())
	{
		sampler
			.SetFilter(GI::Filter::MIN_MAG_MIP_LINEAR)
			.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP });

		geo->CreateAndInitialResource(infra);
	}

	auto genMesh = [&](f32 roughness, f32 metallic, const Vec3f& pos)
	{
		RenderMaterial* material = new RenderMaterial;
		material->mMatAttriSlots[TextureUsage_BaseColor].mConstantValue = Vec4f::Ones() * 0.5f;
		material->mMatAttriSlots[TextureUsage_Metalness].mConstantValue = Vec4f::Ones() * metallic;
		material->mMatAttriSlots[TextureUsage_Roughness].mConstantValue = Vec4f::Ones() * roughness;
		material->mMatAttriSlots[TextureUsage_Normal].mConstantValue = Vec4f{ 0.5f, 0.5f, 1.f, 0.f };

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

Geometry* RenderUtils::GenerateGeometryFromMeshRawData(const MeshRawData* meshRawData)
{
	const i32 vertexCount = meshRawData->mVertexCount;
	const auto& vertexData = meshRawData->mVertexData;
	const u32 vertexTotalStride = std::accumulate(vertexData.begin(), vertexData.end(), 0, [](u32 a, const auto& p) { return a + p.first.mStrideInBytes; });

	std::vector<b8> vertices(vertexTotalStride * vertexCount, b8(0));
	std::vector<GI::InputElementDesc> inputDesc;

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

	u32 vertexStride = 0;
	for (const auto& p : vertexData)
	{
		const VertexAttriMeta& meta = p.first;
		const std::vector<VertexAttriRawData>& attrData = p.second;

		inputDesc.push_back(GI::InputElementDesc()
			.SetSemanticName(names[meta.mType])
			.SetSemanticIndex(meta.mChannelIndex)
			.SetFormat(formats[meta.mStrideInBytes / sizeof(f32)])
			.SetInputSlot(meta.mChannelIndex)
			.SetAlignedByteOffset(vertexStride));

		b8* target = vertices.data() + vertexStride;
		for (i32 i = 0; i < vertexCount; i += 1, target += vertexTotalStride)
		{
			memcpy(target, &attrData[i], meta.mStrideInBytes);
		}

		vertexStride += meta.mStrideInBytes;
	}

	std::vector<u16> indices;
	indices.reserve(meshRawData->mFaces.size() * 3);
	for (const auto& f : meshRawData->mFaces)
	{
		indices.push_back(f[0]);
		indices.push_back(f[1]);
		indices.push_back(f[2]);
	}

	return Geometry::GenerateGeometry(vertices, vertexTotalStride, indices, inputDesc);
}