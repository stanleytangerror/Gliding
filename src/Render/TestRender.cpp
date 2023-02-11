#include "RenderPch.h"
#include "TestRender.h"
#include "RenderModule.h"
// with dx12 dependencies
#include "Geometry.h"
#include "Texture.h"

TestRenderer::TestRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
	, mQuadGeoData(GeometryData::GenerateQuad())
	, mTextureData(Utils::LoadFileContent(R"(D:\Assets\Panorama_of_Marienplatz.dds)"))
{
	
}

void TestRenderer::TestRender(RenderPassManager* passMgr, RenderResourceManager* resMgr)
{
	auto targetSize = Vec2i{ 1600, 900 };

	// declare resources
	auto inputVbResId = resMgr->CreateNamedReadonlyResource("QuadVB", new VertexBufferInitializer(mQuadGeoData));
	auto inputIbResId = resMgr->CreateNamedReadonlyResource("QuadIB", new IndexBufferInitializer(mQuadGeoData));
	auto inputResId = resMgr->CreateNamedReadonlyResource("TestTex", new TextureFromFileInitializer("TestTex", TextureFileExt::DDS, mTextureData));
	auto outputResId = resMgr->CreateSwapChainResource(PresentPortType::MainPort);
	auto samplerId = resMgr->CreateSampler(RHI::SamplerDesc
		{
			RHI::FilterType::MIN_MAG_LINEAR_MIP_POINT,
			RHI::TextureAddressMode::WRAP, RHI::TextureAddressMode::WRAP, RHI::TextureAddressMode::WRAP,
			RHI::ComparisonFunc::NEVER,
			0.f, 0, Vec4f::Zero(), Vec2f::Zero()
		}
	);

	// construct prim
	auto inputPrimitive = InputPrimitiveView
	{
		std::vector<VertexBufferView>{
		{
			inputVbResId,
			RHI::VertexBufferViewDesc{ 0, (u32)mQuadGeoData->mVertexData.size(), mQuadGeoData->mVertexStride }
		}},
		mQuadGeoData->mInputDescs,
		IndexBufferView
		{
			inputIbResId,
			RHI::IndexBufferViewDesc{ 0, (u32)mQuadGeoData->mIndexData.size(), RHI::PixelFormat::R16_UINT }
		},
		RHI::IndexedInstancedParam{ (u32)mQuadGeoData->mIndexData.size(), 1 }
	};


	// construct views
	auto texSrvDesc = RHI::ShaderResourceViewDesc{
		RHI::ViewDimension::TEXTURE2D,
		RHI::PixelFormat::R8G8B8A8_UNORM_SRGB
	};
	texSrvDesc.Texture2D = RHI::SRV::Texture2DView{
						RHI::SRV::Texture1DView {0, 1, 0},
						0
	};

	auto rtRtvDesc = RHI::RenderTargetViewDesc{
					RHI::ViewDimension::TEXTURE2D,
					RHI::PixelFormat::R8G8B8A8_UNORM_SRGB
	};
	rtRtvDesc.Texture2D = RHI::RTV::Texture2DView{
		0, 0
	};

	// construct pass
	auto pass = RenderPass
	{
		"TestPass",
		GraphicProgram
		{
			"res/RootSignature/RootSignature.hlsl",
			"GraphicsRS",
			"res/Shader/CopyTexture.hlsl",
			"res/Shader/CopyTexture.hlsl",
			{ RHI::ProgramMacro{ "SOURCE_PIXEL_UNARY", "color" } }
		},
		inputPrimitive,
		std::map<std::string, ConstantBufferValue>
		{
			{ "RtSize", ConstantBufferValue{ Vec4f{ (f32)targetSize.x(), (f32)targetSize.y(), 1.0f / targetSize.x(), 1.0f / targetSize.y() } } },
			{ "ExposureInfo", ConstantBufferValue{ Vec4f{ -4.0f, 0.0f, 0.0f, 0.0f } } }
		},
		std::map<std::string, ShaderResourceView>
		{
			{ "SourceTex", ShaderResourceView {
				inputResId,
				texSrvDesc
			} }
		},
		std::map<std::string, SamplerView>
		{
			{ "SourceTexSampler", SamplerView { samplerId } },
		},
		std::vector<RenderTargetView>
		{
			RenderTargetView
			{
				outputResId,
				rtRtvDesc
			}
		},
		RHI::ViewPort
		{
			Vec2f { 0.0f, 0.0f },
			Vec2f { targetSize.x(), targetSize.y() },
			Vec2f { 0.0f, 1.0f },
		},
		RHI::Rect
		{
			Vec2f { 0.0f, 0.0f },
			Vec2f { targetSize.x(), targetSize.y() },
		}
	};

	passMgr->AddPass(pass);
}
