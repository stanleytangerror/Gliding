#include "RenderPch.h"
#include "RenderInterface/RenderPass.h"
#include "RenderResource.h"
#include "Geometry.h"

RenderPass RenderPass::GenerateTestRenderPass(RenderResourceManager* resMgr)
{
	auto targetSize = Vec2i{ 1600, 900 };

	const char* texFile = "C:\\Users\\MyComputer\\Desktop\\sky_pano.jpg";
	auto inputResId = resMgr->CreateNamedReadonlyResource(texFile, Utils::LoadFileContent(texFile));
	auto outputResId = resMgr->CreateSwapChainResource();

	GeometryData* quad = GeometryData::GenerateQuad();

	auto inputPrimitive = InputPrimitiveView
	{
		std::vector<VertexBufferView>
		{

		},
		IndexBufferView
		{

		},
		RHI::IndexedInstancedParam
		{

		}
	};

	auto pass = RenderPass
	{
		"ToneMapping",
		GraphicProgram
		{
			L"res/RootSignature/RootSignature.hlsl",
			L"GraphicsRS",
			L"res/Shader/ToneMapping.hlsl",
			L"res/Shader/ToneMapping.hlsl",
		},
		inputPrimitive,
		std::map<std::string, ConstantBufferValue>
		{
			{ "RtSize", ConstantBufferValue { Vec4f{ targetSize.x(), targetSize.y(), 1.0f / targetSize.x(), 1.0f / targetSize.y() }} },
			{ "ExposureInfo", ConstantBufferValue { Vec4f{ -4.0f, 0.0f, 0.0f, 0.0f } } }
		},
		std::map<std::string, ShaderResourceView>
		{
			{ "SceneHdr", ShaderResourceView {
				inputResId,
				RHI::ShaderResourceViewDesc{
					RHI::ViewDimention::TEXTURE2D,
					RHI::PixelFormat::R8G8B8A8_UNORM_SRGB
				}
			} }
		},
		std::vector<RenderTargetView>
		{
			RenderTargetView
			{
				outputResId,
				RHI::RenderTargetViewDesc {
					RHI::ViewDimention::TEXTURE2D,
					RHI::PixelFormat::R8G8B8A8_UNORM_SRGB
				}
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

	return pass;
}
