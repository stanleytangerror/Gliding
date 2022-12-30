#include "RenderPch.h"
#include "ImGuiRenderer.h"
#include "RenderModule.h"
#include "D3D12Backend/D3D12CommandContext.h"
#include "D3D12Backend/D3D12ResourceView.h"
#include "D3D12Backend/D3D12PipelinePass.h"
#include "D3D12Backend/D3D12Resource.h"
#include "D3D12Backend/D3D12Geometry.h"
#include "D3D12Backend/D3D12Texture.h"
#include "D3D12Backend/D3D12Headers.h"
#include "ImGuiIntegration/ImGuiIntegration.h"
#include "imgui.h"

ImGuiRenderer::ImGuiRenderer(RenderModule* renderModule)
	: mRenderModule(renderModule)
{
	D3D12Backend::D3D12Device* device = mRenderModule->GetDevice();
	mImGuiSampler = new D3D12Backend::SamplerView(device, D3D12_FILTER_MIN_MAG_MIP_LINEAR, { D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP });

	unsigned char* pixels = nullptr;
	i32 width = 0, height = 0, bytesPerPixel = 0;
	ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);
	Assert(bytesPerPixel == 4);

	u32 uploadPitch = Math::Align(width * 4, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	u32 uploadSize = height * uploadPitch;

	std::vector<b8> fontAtlas(uploadSize);
	for (i32 y = 0; y < height; y++)
	{
		memcpy((void*)((uintptr_t)fontAtlas.data() + y * uploadPitch), pixels + y * width * 4, width * 4);
	}

	mFontAtlas = new D3D12Texture(device, DXGI_FORMAT_R8G8B8A8_UNORM, fontAtlas, { width, height, 1 }, 1, "ImGuiFontAtlas");
}

void ImGuiRenderer::TickFrame(Timer* timer)
{

}

void ImGuiRenderer::Render(D3D12Backend::GraphicsContext* context, D3D12Backend::RenderTargetView* target, ImDrawData* uiData)
{
	if (!mFontAtlas->IsD3DResourceReady())
	{
		mFontAtlas->Initial(context);
		ImGui::GetIO().Fonts->SetTexID(mFontAtlas->GetSrv());
	}

	RENDER_EVENT(context, ImGuiRenderer::Render);

	D3D12Backend::D3D12Device* device = mRenderModule->GetDevice();

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

	std::unique_ptr<D3D12Geometry> geo;
	geo.reset(D3D12Geometry::GenerateGeometry(device, vertexBuffer, indexBuffer,
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32_UINT,		 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		}));

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

			const RECT scissorRect = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };

			D3D12Backend::ShaderResourceView* srv = reinterpret_cast<D3D12Backend::ShaderResourceView*>(cmd->GetTexID());

			D3D12Backend::GraphicsPass pass(context);

			pass.mRootSignatureDesc.mFile = "res/RootSignature/RootSignature.hlsl";
			pass.mRootSignatureDesc.mEntry = "GraphicsRS";
			pass.mVsFile = "res/Shader/ImGui.hlsl";
			pass.mPsFile = "res/Shader/ImGui.hlsl";
			pass.mShaderMacros.push_back(D3D12Backend::ShaderMacro{ "USE_TEXTURE", srv ? "1" : "0" });

			D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc = pass.PsoDesc();
			{
				// Create the blending setup
				{
					D3D12_BLEND_DESC& desc = psoDesc.BlendState;
					desc.AlphaToCoverageEnable = false;
					desc.RenderTarget[0].BlendEnable = true;
					desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
					desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
					desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
					desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
					desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
					desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
					desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
				}

				// Create the rasterizer state
				{
					D3D12_RASTERIZER_DESC& desc = psoDesc.RasterizerState;
					desc.FillMode = D3D12_FILL_MODE_SOLID;
					desc.CullMode = D3D12_CULL_MODE_NONE;
					desc.FrontCounterClockwise = FALSE;
					desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
					desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
					desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
					desc.DepthClipEnable = true;
					desc.MultisampleEnable = FALSE;
					desc.AntialiasedLineEnable = FALSE;
					desc.ForcedSampleCount = 0;
					desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
				}

				// Create depth-stencil State
				{
					D3D12_DEPTH_STENCIL_DESC& desc = psoDesc.DepthStencilState;
					desc.DepthEnable = false;
					desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
					desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
					desc.StencilEnable = false;
					desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
					desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
					desc.BackFace = desc.FrontFace;
				}

				psoDesc.InputLayout = { geo->mInputDescs.data(), u32(geo->mInputDescs.size()) };
			}

			const Vec3i& targetSize = target->GetResource()->GetSize();
			pass.mRts[0] = target;
			pass.mViewPort = CD3DX12_VIEWPORT(0.f, 0.f, targetSize.x(), targetSize.y());
			pass.mScissorRect = scissorRect;

			pass.mVbvs.clear();
			pass.mVbvs.push_back(geo->mVbv);
			pass.mIbv = geo->mIbv;
			pass.mIndexCount = cmd->ElemCount;
			pass.mIndexStartLocation = indexOffset + cmd->IdxOffset;
			pass.mVertexStartLocation = vertexOffset + cmd->VtxOffset;

			if (srv)
			{
				pass.AddSrv("SourceTex", srv);
				pass.AddSampler("SourceTexSampler", mImGuiSampler);
			}

			pass.AddCbVar("WvpMat", wvpMat);

			pass.Draw();
		}
		indexOffset += cmdList->IdxBuffer.Size;
		vertexOffset += cmdList->VtxBuffer.Size;
	}
}
