#include "RenderPch.h"
#include "ImGuiRenderer.h"
#include "RenderModule.h"
#include "Geometry.h"
#include "Texture.h"
#include "ImGuiIntegration/ImGuiIntegration.h"
#include "imgui.h"

ImGuiRenderer::ImGuiRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	mImGuiSampler
		.SetFilter(GI::Filter::MIN_MAG_MIP_LINEAR)
		.SetAddress({ GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP, GI::TextureAddressMode::WRAP });

	unsigned char* pixels = nullptr;
	i32 width = 0, height = 0, bytesPerPixel = 0;
	ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);
	Assert(bytesPerPixel == 4);

	u32 uploadPitch = Math::Align(width * 4, GI::GetDataPitchAlignment());
	u32 uploadSize = height * uploadPitch;

	std::vector<b8> fontAtlas(uploadSize);
	for (i32 y = 0; y < height; y++)
	{
		memcpy((void*)((uintptr_t)fontAtlas.data() + y * uploadPitch), pixels + y * width * 4, width * 4);
	}

	mFontAtlas.reset(new InMemoryTexture(mRenderModule->GetGraphicsInfra(), GI::Format::FORMAT_R8G8B8A8_UNORM, fontAtlas, { width, height, 1 }, 1, "ImGuiFontAtlas"));
}

void ImGuiRenderer::TickFrame(Timer* timer)
{

}

void ImGuiRenderer::Render(GI::IGraphicsInfra* infra, const GI::RtvUsage& target, ImDrawData* uiData)
{
	if (!mFontAtlas->IsD3DResourceReady())
	{
		mFontAtlas->CreateAndInitialResource(infra);

		auto resource = mFontAtlas->GetResource();
		mFontAtlasSrvDesc = GI::SrvUsage(resource);
		mFontAtlasSrvDesc
			.SetFormat(resource->GetFormat())
			.SetViewDimension(GI::SrvDimension::TEXTURE2D)
			.SetTexture2D_MipLevels(resource->GetMipLevelCount());

		ImGui::GetIO().Fonts->SetTexID(&mFontAtlasSrvDesc);
	}

	RENDER_EVENT(infra, ImGuiRenderer::Render);

	// Avoid rendering when minimized
	if (!uiData || uiData->CmdListsCount == 0 || uiData->DisplaySize.x <= 0.0f || uiData->DisplaySize.y <= 0.0f) { return; }

	Mat44f wvpMat;
	{
		/* imgui position space in screen:
		 * x+: screen right
		 * y+: screen down
		 */
		const Vec2f size = { uiData->DisplaySize.x, uiData->DisplaySize.y };
		const Vec2f pos = Vec2f{ uiData->DisplayPos.x, uiData->DisplayPos.y } + 0.5f * size;

		Math::CameraTransformf camTrans;
		camTrans.AlignCamera(
			Math::Axis3DDir<f32>(Math::Axis3D_Zp),
			Math::Axis3DDir<f32>(Math::Axis3D_Yn),
			Math::Axis3DDir<f32>(Math::Axis3D_Xp));
		camTrans.MoveCamera(Vec3f{ pos.x(), pos.y(), 0.f });
		
		Math::OrthographicProjectionf proj;
		proj.mNear = 0.f;
		proj.mFar = 1.f;
		proj.mViewWidth = size.x();
		proj.mViewHeight = size.y(); 
		
		wvpMat = proj.ComputeProjectionMatrix() * camTrans.ComputeViewMatrix();
	}

	// Create and grow buffers if needed
	std::vector<ImDrawVert> vertexBuffer(uiData->TotalVtxCount);
	std::vector<ImDrawIdx> indexBuffer(uiData->TotalIdxCount);

	i32 vertexOffset = 0;
	i32 indexOffset = 0;
	for (i32 n = 0; n < uiData->CmdListsCount; n++)
	{
		const ImDrawList* cmdList = uiData->CmdLists[n];

		memcpy(vertexBuffer.data() + vertexOffset, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.size() * sizeof(ImDrawVert));
		vertexOffset += cmdList->VtxBuffer.size();
		memcpy(indexBuffer.data() + indexOffset, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.size() * sizeof(ImDrawIdx));
		indexOffset += cmdList->IdxBuffer.size();

		Assert(uiData->TotalVtxCount >= vertexOffset);
		Assert(uiData->TotalIdxCount >= indexOffset);
	}

	std::unique_ptr<Geometry> geo;

	geo.reset(Geometry::GenerateGeometry(vertexBuffer, indexBuffer,
		{
			GI::InputElementDesc()
				.SetSemanticName("POSITION")
				.SetFormat(GI::Format::FORMAT_R32G32_FLOAT)
				.SetAlignedByteOffset(IM_OFFSETOF(ImDrawVert, pos)),
			GI::InputElementDesc()
				.SetSemanticName("TEXCOORD")
				.SetFormat(GI::Format::FORMAT_R32G32_FLOAT)
				.SetAlignedByteOffset(IM_OFFSETOF(ImDrawVert, uv)),
			GI::InputElementDesc()
				.SetSemanticName("COLOR")
				.SetFormat(GI::Format::FORMAT_R32_UINT)
				.SetAlignedByteOffset(IM_OFFSETOF(ImDrawVert, col))
		}));

	geo->CreateAndInitialResource(infra);

	// Render command lists
	vertexOffset = 0;
	indexOffset = 0;
	const ImVec2& clipOffset = uiData->DisplayPos;
	for (int n = 0; n < uiData->CmdListsCount; n++)
	{
		const ImDrawList* cmdList = uiData->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* cmd = &cmdList->CmdBuffer[cmd_i];
			Assert(cmd->UserCallback == nullptr);

			const ImVec2 clip_min(cmd->ClipRect.x - clipOffset.x, cmd->ClipRect.y - clipOffset.y);
			const ImVec2 clip_max(cmd->ClipRect.z - clipOffset.x, cmd->ClipRect.w - clipOffset.y);
			if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) { continue; }

			const Math::Rect scissorRect = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };

			const auto* srv = reinterpret_cast<const GI::SrvUsage*>(cmd->GetTexID());

			GI::GraphicsPass pass;

			pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
			pass.mRootSignatureDesc.mEntry = "GraphicsRS";
			pass.mVsFile = "res/Shader/ImGui.hlsl";
			pass.mPsFile = "res/Shader/ImGui.hlsl";
			pass.mShaderMacros.push_back(GI::ShaderMacro{ "USE_TEXTURE", srv ? "1" : "0" });

			pass.mBlendDesc.SetAlphaToCoverageEnable(false);
			pass.mBlendDesc.RtBlendDesc[0]
				.SetBlendEnable(true)
				.SetSrcBlend(GI::Blend::SRC_ALPHA)
				.SetDestBlend(GI::Blend::INV_SRC_ALPHA)
				.SetBlendOp(GI::BlendOp::ADD)
				.SetSrcBlendAlpha(GI::Blend::ONE)
				.SetDestBlendAlpha(GI::Blend::INV_SRC_ALPHA)
				.SetBlendOpAlpha(GI::BlendOp::ADD);

			pass.mDepthStencilDesc
				.SetDepthEnable(false)
				.SetStencilEnable(false);

			pass.mInputLayout = geo->mVertexElementDescs;

			const Vec3u& targetSize = target.GetResource()->GetSize();
			pass.SetRtv(0, target);
			pass.mViewPort.SetWidth(targetSize.x()).SetHeight(targetSize.y());
			pass.mScissorRect = scissorRect;

			pass.PushVbv(geo->GetVbvDesc());
			pass.SetIbv(geo->GetIbvDesc());
			pass.mIndexCount = cmd->ElemCount;
			pass.mIndexStartLocation = indexOffset + cmd->IdxOffset;
			pass.mVertexStartLocation = vertexOffset + cmd->VtxOffset;

			if (srv)
			{
				pass.AddSrv("SourceTex", *srv);
				pass.AddSampler("SourceTexSampler", mImGuiSampler);
			}

			pass.AddCbVar("WvpMat", wvpMat);

			infra->GetRecorder()->AddGraphicsPass(pass);
		}
		indexOffset += cmdList->IdxBuffer.Size;
		vertexOffset += cmdList->VtxBuffer.Size;
	}
}
