using Interop;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using RenderTypes;

namespace CSLauncher
{
    public class RenderPass
    {
		public string Name;
        public GraphicProgram Program;
        public InputPrimitiveView InputPrimitive;
        public Dictionary<string, ConstantBufferView> ConstBuffers;
        public Dictionary<string, ShaderResourceView> Srvs;
        public RenderTargetView[] Rtvs;
		public ViewPort ViewPort;
		public Rect ScissorRect;

		public static RenderPass GenerateRenderPass()
		{
			var targetSize = new Vec2u { x = 1600, y = 900 };
			var inputTexture = new ExternalResource { File = "C:\\Users\\MyComputer\\Desktop\\sky_pano.jpg" };

			var outputTextureDesc = new CommitedResourceDesc
			{
				Dimension = ResourceDimention.Texture2D,
				Format = PixelFormat.R8G8B8A8_UNORM_SRGB,
				Size = new ResourceSize { Width = targetSize.x, Height = targetSize.y, DepthOrArraySize = 1 },
				MipLevels = 1
			};

            var outputTexture = new TransientResource
			{

			};
			var inputPrimitive = new InputPrimitiveView
			{
				Vbvs = new VertexBufferView[]
				{

				},
				Ibv = new IndexBufferView
				{

				},
				Param = new IndexedInstancedParam
				{

				}
			};

			RenderPass pass = new RenderPass()
			{
				Name = "ToneMapping",
				Program = new GraphicProgram
				{
					RsName = "res/RootSignature/RootSignature.hlsl",
					RsEntry = "GraphicsRS",
					VsName = "res/Shader/ToneMapping.hlsl",
					PsName = "res/Shader/ToneMapping.hlsl",
				},
				ConstBuffers = new Dictionary<string, ConstantBufferView>
				{
					{ "RtSize", new ConstantBufferView() { Value = new Vec4f{ x = targetSize.x, y = targetSize.y, z = 1.0f / targetSize.x, w = 1.0f / targetSize.y } } },
					{ "ExposureInfo", new ConstantBufferView() { Value = new Vec4f{ x = -4.0f, y = 0.0f, z = 0.0f, w = 0.0f } } }
				},
				Srvs = new Dictionary<string, ShaderResourceView>
				{
					{ "SceneHdr", new ShaderResourceView {
						ResourceId = inputTexture.ResourceId,
						SrvDesc = new ShaderResourceViewDesc{
							Dimension = ViewDimension.TEXTURE2D,
							Format = PixelFormat.R8G8B8A8_UNORM_SRGB }
					} }
				},
				Rtvs = new RenderTargetView[]
				{
					new RenderTargetView
					{
						ResourceId = outputTexture.ResourceId,
						RtvDesc = new RenderTargetViewDesc{
							Dimension = ViewDimension.TEXTURE2D,
							Format = PixelFormat.R8G8B8A8_UNORM_SRGB }
					}
				},
				ViewPort = new ViewPort
				{
					LeftTop = new Vec2f { x = 0.0f, y = 0.0f },
					Size = new Vec2f { x = targetSize.x, y = targetSize.y },
					DepthRange = new Vec2f { x = 0.0f, y = 1.0f },
				},
				ScissorRect = new Rect
				{
					LeftTop = new Vec2f { x = 0.0f, y = 0.0f },
					RightBottom = new Vec2f { x = targetSize.x, y = targetSize.y },
				},
				InputPrimitive = inputPrimitive
			};

            return pass;
        }
    }
}
