#include "RenderPch.h"
#include "RenderUtils.h"
#include "D3D12Backend/D3D12PipelinePass.h"
#include "Geometry.h"
#include "D3D12Backend/D3D12CommandContext.h"
#include "D3D12Backend/D3D12ResourceView.h"
#include "D3D12Backend/D3D12Resource.h"
#include "RenderTarget.h"
#include "World/Scene.h"
#include "RenderMaterial.h"
#include "Texture.h"

namespace
{
	D3D12_SAMPLER_DESC ToD3DSamplerDesc(const TextureSamplerType& type)
	{
		D3D12_SAMPLER_DESC result = {};

		auto mapType = [](SamplerAddrMode mode)
		{
			switch (mode)
			{
			case TextureSamplerType_Clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			case TextureSamplerType_Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			case TextureSamplerType_Wrap: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			case TextureSamplerType_Decal:
			default:  Assert(false); return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			}
		};

		result.AddressU = mapType(type[0]);
		result.AddressV = mapType(type[1]);
		result.AddressW = mapType(type[2]);

		return result;
	}
}

void RenderUtils::CopyTexture(D3D12Backend::GraphicsContext* context,
	D3D12Backend::RenderTargetView* target, const Vec2f& targetOffset, const Vec2f& targetRect, 
	D3D12Backend::ShaderResourceView* source, D3D12Backend::SamplerView* sourceSampler, const char* sourcePixelUnary)
{
	static Geometry* quad = Geometry::GenerateQuad(context->GetDevice());

	D3D12Backend::GraphicsPass pass(context);

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/CopyTexture.hlsl";
	pass.mPsFile = "res/Shader/CopyTexture.hlsl";
	pass.mShaderMacros.push_back(D3D12Backend::ShaderMacro{ "SOURCE_PIXEL_UNARY", sourcePixelUnary ? sourcePixelUnary : "color"});

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { quad->mInputDescs.data(), u32(quad->mInputDescs.size()) };
	}

	const Vec3i& targetSize = target->GetResource()->GetSize();
	pass.mRts[0] = target;
	pass.mViewPort = CD3DX12_VIEWPORT(targetOffset.x(), targetOffset.y(), targetRect.x(), targetRect.y());
	pass.mScissorRect = { 0, 0, targetSize.x(), targetSize.y() };

	pass.mVbvs.clear();
	pass.mVbvs.push_back(quad->mVbv);
	pass.mIbv = quad->mIbv;
	pass.mIndexCount = quad->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("RtSize", Vec4f{ targetRect.x(), targetRect.y(), 1.f / targetRect.x(), 1.f / targetRect.y() });
	pass.AddSrv("SourceTex", source);
	pass.AddSampler("SourceTexSampler", sourceSampler);

	pass.Draw();
}

void RenderUtils::CopyTexture(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target, D3D12Backend::ShaderResourceView* source, D3D12Backend::SamplerView* sourceSampler)
{
	const auto& targetSize = target->GetResource()->GetSize();
	CopyTexture(context, target, Vec2f::Zero(), { targetSize.x(), targetSize.y() }, source, sourceSampler);
}

void GaussianBlur1D(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target, D3D12Backend::ShaderResourceView* source, i32 kernelSizeInPixel, D3D12Backend::SamplerView* sampler, Geometry* quad, bool isHorizontal)
{
	auto NormalDistPdf = [](f32 x, f32 stdDev) { return exp(-0.5f * (x * x / stdDev / stdDev) / stdDev) / Math::Sqrt(2.f * Math::Pi<f32>()); };

	const auto& size = source->GetResource()->GetSize();
	const auto& weight4fSize = (kernelSizeInPixel + 1 + 3) / 4;

	D3D12Backend::GraphicsPass pass(context);

	pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
	pass.mRootSignatureDesc.mEntry = "GraphicsRS";
	pass.mVsFile = "res/Shader/GaussianBlur.hlsl";
	pass.mPsFile = "res/Shader/GaussianBlur.hlsl";
	pass.mShaderMacros.push_back(D3D12Backend::ShaderMacro{ "WEIGHT_SIZE", Utils::FormatString("%d", weight4fSize) });
	pass.mShaderMacros.push_back(D3D12Backend::ShaderMacro{ isHorizontal ? "HORIZONTAL" : "VERTICAL", "1"});

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = pass.PsoDesc();
	{
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		desc.InputLayout = { quad->mInputDescs.data(), u32(quad->mInputDescs.size()) };
	}

	pass.mRts[0] = target;
	pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, f32(size.x()), f32(size.y()));
	pass.mScissorRect = { 0, 0, size.x(), size.y() };

	pass.mVbvs.clear();
	pass.mVbvs.push_back(quad->mVbv);
	pass.mIbv = quad->mIbv;
	pass.mIndexCount = quad->mIbv.SizeInBytes / sizeof(u16);

	pass.AddCbVar("BlurTargetSize", Vec4f{ f32(size.x()), f32(size.y()), 1.f / size.x(), 1.f / size.y() });
	pass.AddSrv("SourceTex", source);
	pass.AddSampler("SourceTexSampler", sampler);

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

	pass.Draw();
}

void RenderUtils::GaussianBlur(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target, D3D12Backend::ShaderResourceView* source, i32 kernelSizeInPixel)
{
	static D3D12Backend::SamplerView* sampler = new D3D12Backend::SamplerView(context->GetDevice(), D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });
	static Geometry* quad = Geometry::GenerateQuad(context->GetDevice());

	std::unique_ptr<RenderTarget> interRt = std::make_unique<RenderTarget>(context->GetDevice(), source->GetResource()->GetSize(), source->GetFormat(), "GaussianBlurIntermediateRT");

	RENDER_EVENT(context, GaussianBlur);
	GaussianBlur1D(context, interRt->GetRtv(), source, kernelSizeInPixel, sampler, quad, true);
	GaussianBlur1D(context, target, interRt->GetSrv(), kernelSizeInPixel, sampler, quad, false);
}

TransformNode<std::pair<
	std::unique_ptr<Geometry>,
	std::shared_ptr<RenderMaterial>>>*
RenderUtils::FromSceneRawData(D3D12Backend::D3D12Device* device, SceneRawData* sceneRawData)
{
	auto result = new TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>;

	std::map<std::string, Texture*> textures;
	for (const auto& [texPath, texRawData] : sceneRawData->mTextures)
	{
		if (texRawData)
		{
			textures[texPath] = new Texture(texPath.c_str(), texRawData->mRawData);
		}
	}
	std::map<TextureSamplerType, D3D12Backend::SamplerView*> samplers;
	for (const TextureSamplerType& samplerType : sceneRawData->mSamplers)
	{
		samplers[samplerType] = new D3D12Backend::SamplerView(device, ToD3DSamplerDesc(samplerType));
	}

	std::vector<std::shared_ptr<RenderMaterial>> materials;
	for (const auto& mat : sceneRawData->mMaterials)
	{
		materials.emplace_back(RenderMaterial::GenerateRenderMaterialFromRawData(mat, sceneRawData, textures, samplers));
	}
	for (MeshRawData* mesh : sceneRawData->mMeshes)
	{
		Geometry* geo = GenerateGeometryFromMeshRawData(device, mesh);
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
	std::shared_ptr<RenderMaterial>>>* RenderUtils::GenerateMaterialProbes(D3D12Backend::D3D12Device* device)
{
	auto result = new TransformNode<std::pair<
		std::unique_ptr<Geometry>,
		std::shared_ptr<RenderMaterial>>>;

	Geometry* geo = Geometry::GenerateSphere(device, 40);
	D3D12Backend::SamplerView* sampler = new D3D12Backend::SamplerView(device, D3D12_FILTER_MIN_MAG_MIP_LINEAR, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });

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

Geometry* RenderUtils::GenerateGeometryFromMeshRawData(D3D12Backend::D3D12Device* device, const MeshRawData* meshRawData)
{
	const i32 vertexCount = meshRawData->mVertexCount;
	const auto& vertexData = meshRawData->mVertexData;
	const u32 vertexTotalStride = std::accumulate(vertexData.begin(), vertexData.end(), 0, [](u32 a, const auto& p) { return a + p.first.mStrideInBytes; });

	std::vector<b8> vertices(vertexTotalStride * vertexCount, b8(0));
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputDesc;

	static const char* names[] =
	{
		"POSITION",
		"NORMAL",
		"TANGENT",
		"BINORMAL",
		"TEXCOORD",
		"COLOR"
	};

	static const DXGI_FORMAT formats[] =
	{
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
	};

	u32 vertexStride = 0;
	for (const auto& p : vertexData)
	{
		const VertexAttriMeta& meta = p.first;
		const std::vector<VertexAttriRawData>& attrData = p.second;

		inputDesc.push_back(D3D12_INPUT_ELEMENT_DESC{
			names[meta.mType],
			meta.mChannelIndex,
			formats[meta.mStrideInBytes / sizeof(f32)],
			meta.mChannelIndex,
			vertexStride,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 });

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

	return Geometry::GenerateGeometry(device, vertices, vertexTotalStride, indices, inputDesc);
}