#include "Includes.h"
#include "CppUnitTest.h"
#include "../Common/Serialization.h"
#include "../Common/GraphicsInfrastructure.h"
#include <Windows.h> // Added header
#include <span>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ch = std::chrono;

namespace UnitTest
{
	TEST_CLASS(GraphicsBackendTest)
	{
	protected:
		static inline HMODULE msPlatformModule;
		static inline Platform::CreateNativeWindow* msCreateNativeWindow = nullptr;
		static inline Platform::DestroyNativeWindow* msDestroyNativeWindow = nullptr;

		static inline HMODULE msGraphicsBackendModule;
		static inline GI::CreateGraphicsInfra* msCreateGraphicsInfra = nullptr;

		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
#ifdef _DEBUG
			msPlatformModule = LoadLibrary(L"WindowsPlatform_Debug_x64.dll");
			msGraphicsBackendModule = LoadLibrary(L"D3D12Backend_Debug_x64.dll");
#else
			msPlatformModule = LoadLibrary(L"WindowsPlatform_Release_x64.dll");
			msGraphicsBackendModule = LoadLibrary(L"D3D12Backend_Release_x64.dll");
#endif

			msCreateNativeWindow = reinterpret_cast<Platform::CreateNativeWindow*>(GetProcAddress(msPlatformModule, "CreateNativeWindow"));
			msDestroyNativeWindow = reinterpret_cast<Platform::DestroyNativeWindow*>(GetProcAddress(msPlatformModule, "DestroyNativeWindow"));

			msCreateGraphicsInfra = reinterpret_cast<GI::CreateGraphicsInfra*>(GetProcAddress(msGraphicsBackendModule, "CreateGraphicsInfra"));

			SetCurrentDirectory(L"../../..");
		}

		TEST_CLASS_CLEANUP(ClassCleanup)
		{
			FreeLibrary(msPlatformModule);
			FreeLibrary(msGraphicsBackendModule);
		}

	public:
		TEST_METHOD(CreateDevice_Succeed)
		{
			auto graphicsInfra = msCreateGraphicsInfra();
			Assert::IsTrue(graphicsInfra != nullptr);
			delete graphicsInfra;
		}

		TEST_METHOD(HelloTriangle_Succeed)
		{
			auto window = msCreateNativeWindow(L"TestWindow", Vec2u{ 640, 360 });
			auto graphicsInfra = msCreateGraphicsInfra();

			const auto windowInfo = window->GetInfo();
			const auto& windowSize = windowInfo.mSize;

			graphicsInfra->AdaptToWindow(windowInfo, 2);

			auto start = ch::high_resolution_clock::now();
			while (ch::duration_cast<ch::seconds>(ch::high_resolution_clock::now() - start).count() < 3)
			{
				graphicsInfra->StartFrame();

				struct DataType { Vec2f pos; Vec3f color; };
				std::vector<DataType> points =
				{
					{ Vec2f{ -1.f, 0.f },     Vec3f{ 1.f, 0.f, 0.f } },
					{ Vec2f{ 1.f, 0.f },     Vec3f{ 0.f, 1.f, 0.f } },
					{ Vec2f{ 0.f, 1.732f }, Vec3f{ 0.f, 0.f, 1.f } },
				};
				auto vertices = std::as_bytes(std::span(points));

				auto vb = graphicsInfra->CreateMemoryResource(
					GI::MemoryResourceDesc::Buffer2(vertices.size(), false, false, "Vertices")
					.SetInitState(GI::ResourceState::STATE_GENERIC_READ)
					.SetHeapType(GI::HeapType::UPLOAD));
				graphicsInfra->CopyToUploadBufferResource(vb.get(), vertices);

				GI::VbvUsage vbv;
				vbv.mResource = vb.get();
				vbv.mDesc
					.SetSizeInBytes(vertices.size())
					.SetStrideInBytes(sizeof(Vec2f));

				std::vector<u16> ids = { 0, 1, 2 };
				auto indices = std::as_bytes(std::span(ids));

				auto ib = graphicsInfra->CreateMemoryResource(
					GI::MemoryResourceDesc::Buffer2(indices.size(), false, false, "Indices")
					.SetInitState(GI::ResourceState::STATE_GENERIC_READ)
					.SetHeapType(GI::HeapType::UPLOAD));
				graphicsInfra->CopyToUploadBufferResource(ib.get(), indices);

				GI::IbvUsage ibv;
				ibv.mResource = ib.get();
				ibv.mDesc
					.SetFormat(GI::Format::FORMAT_R16_UINT)
					.SetSizeInBytes(indices.size());

				std::vector<GI::InputElementDesc> inputDescs = {
					GI::InputElementDesc()
						.SetSemanticName("POSITION")
						.SetSemanticIndex(0)
						.SetFormat(GI::Format::FORMAT_R32G32_FLOAT)
						.SetInputSlot(0)
						.SetAlignedByteOffset(0)
						.SetInputSlotClass(GI::InputClassification::PER_VERTEX_DATA)
						.SetInstanceDataStepRate(0),
					GI::InputElementDesc()
						.SetSemanticName("COLOR")
						.SetSemanticIndex(0)
						.SetFormat(GI::Format::FORMAT_R32G32B32_FLOAT)
						.SetInputSlot(0)
						.SetAlignedByteOffset(sizeof(Vec2f))
						.SetInputSlotClass(GI::InputClassification::PER_VERTEX_DATA)
						.SetInstanceDataStepRate(0)
				};

				auto rt = graphicsInfra->GetWindowBackBuffer(windowInfo.mNativeHandle);

				GI::RtvUsage rtv;
				rtv.mResource = rt;
				rtv.mDesc
					.SetViewDimension(GI::RtvDimension::TEXTURE2D)
					.SetFormat(GI::Format::FORMAT_R8G8B8A8_UNORM);

				graphicsInfra->GetRecorder()->AddClearOperation(rtv, Vec4f{ 0.5f, 0.5f, 0.5f, 1.0f });

				GI::GraphicsPass pass;
				{
					pass.SetShader("HelloTriangle");

					pass.SetupDepthStencil()
						.SetDepthEnable(false)
						.SetStencilEnable(false);

					pass.SetRtv(0, rtv);
					pass.mViewPort.SetTopLeftX(0).SetTopLeftY(0).SetWidth(windowSize.x()).SetHeight(windowSize.y());
					pass.mScissorRect = { 0, 0, i32(windowSize.x()), i32(windowSize.y()) };

					pass.SetGeometry(vbv, 0, inputDescs, ibv, 0, ids.size());

					pass.AddCb4f("rtSize", Vec4f{ f32(windowSize.x()), f32(windowSize.y()), 1.f / windowSize.x(), 1.f / windowSize.y() });
				}

				graphicsInfra->GetRecorder()->AddGraphicsPass(pass);

				graphicsInfra->EndFrame();

				graphicsInfra->Present();
			}

			delete graphicsInfra;
			msDestroyNativeWindow(window);
		}

		TEST_METHOD(HelloTexture_Succeed)
		{
			auto window = msCreateNativeWindow(L"TestWindow", Vec2u{ 640, 360 });
			auto graphicsInfra = msCreateGraphicsInfra();

			const auto windowInfo = window->GetInfo();
			const auto& windowSize = windowInfo.mSize;

			graphicsInfra->AdaptToWindow(windowInfo, 2);

			auto texturePath = R"(res\Scene\lion.dds)";
			auto image = graphicsInfra->CreateFromImageMemory(
				Utils::GetTextureExtension(texturePath), 
				Utils::LoadFileContent(texturePath), 
				texturePath);

			auto start = ch::high_resolution_clock::now();
			while (ch::duration_cast<ch::seconds>(ch::high_resolution_clock::now() - start).count() < 3)
			{
				graphicsInfra->StartFrame();

				auto texureResource = graphicsInfra->CreateMemoryResource(image->GetResourceDesc());
				graphicsInfra->InitialMemoryResourceFromImage(texureResource.get(), *image);
				const auto& imageDesc = image->GetResourceDesc();

				GI::SrvUsage srv;
				srv.mResource = texureResource.get();
				srv.mDesc
					.SetViewDimension(GI::SrvDimension::TEXTURE2D)
					.SetFormat(GI::GetSrvFormat(imageDesc.GetFormat()))
					.SetTexture2D_MipLevels(imageDesc.GetMipLevels());

				std::vector<Vec2f> points =
				{
					Vec2f{ 0.f, 0.f },
					Vec2f{ 1.f, 0.f },
					Vec2f{ 1.f, 1.f },
					Vec2f{ 0.f, 1.f },
				};
				auto vertices = std::as_bytes(std::span(points));

				auto vb = graphicsInfra->CreateMemoryResource(
					GI::MemoryResourceDesc::Buffer2(vertices.size(), false, false, "Vertices")
					.SetInitState(GI::ResourceState::STATE_GENERIC_READ)
					.SetHeapType(GI::HeapType::UPLOAD));
				graphicsInfra->CopyToUploadBufferResource(vb.get(), vertices);

				GI::VbvUsage vbv;
				vbv.mResource = vb.get();
				vbv.mDesc
					.SetSizeInBytes(vertices.size())
					.SetStrideInBytes(sizeof(Vec2f));

				GI::SamplerDesc sampler;
				sampler
					.SetAddressXYZ(GI::TextureAddressMode::WRAP)
					.SetFilter(GI::Filter::MIN_MAG_MIP_LINEAR);

				std::vector<u16> ids = { 0, 1, 2, 0, 2, 3 };
				auto indices = std::as_bytes(std::span(ids));

				auto ib = graphicsInfra->CreateMemoryResource(
					GI::MemoryResourceDesc::Buffer2(indices.size(), false, false, "Indices")
					.SetInitState(GI::ResourceState::STATE_GENERIC_READ)
					.SetHeapType(GI::HeapType::UPLOAD));
				graphicsInfra->CopyToUploadBufferResource(ib.get(), indices);

				GI::IbvUsage ibv;
				ibv.mResource = ib.get();
				ibv.mDesc
					.SetFormat(GI::Format::FORMAT_R16_UINT)
					.SetSizeInBytes(indices.size());

				std::vector<GI::InputElementDesc> inputDescs = {
					GI::InputElementDesc()
						.SetSemanticName("POSITION")
						.SetSemanticIndex(0)
						.SetFormat(GI::Format::FORMAT_R32G32_FLOAT)
						.SetInputSlot(0)
						.SetAlignedByteOffset(0)
						.SetInputSlotClass(GI::InputClassification::PER_VERTEX_DATA)
						.SetInstanceDataStepRate(0)
				};

				auto rt = graphicsInfra->GetWindowBackBuffer(windowInfo.mNativeHandle);

				GI::RtvUsage rtv;
				rtv.mResource = rt;
				rtv.mDesc
					.SetViewDimension(GI::RtvDimension::TEXTURE2D)
					.SetFormat(GI::Format::FORMAT_R8G8B8A8_UNORM);

				graphicsInfra->GetRecorder()->AddClearOperation(rtv, Vec4f{ 0.5f, 0.5f, 0.5f, 1.0f });

				GI::GraphicsPass pass;
				{
					pass.SetShader("HelloTexture");

					pass.SetupDepthStencil()
						.SetDepthEnable(false)
						.SetStencilEnable(false);

					pass.SetRtv(0, rtv);
					pass.mViewPort.SetTopLeftX(0).SetTopLeftY(0).SetWidth(windowSize.x()).SetHeight(windowSize.y());
					pass.mScissorRect = { 0, 0, i32(windowSize.x()), i32(windowSize.y()) };

					pass.SetGeometry(vbv, 0, inputDescs, ibv, 0, ids.size());

					pass.AddCb4f("rtSize", Vec4f{ f32(windowSize.x()), f32(windowSize.y()), 1.f / windowSize.x(), 1.f / windowSize.y() });

					pass.AddSrv("inputTex", srv);
					pass.AddSampler("inputTexSampler", sampler);
				}

				graphicsInfra->GetRecorder()->AddGraphicsPass(pass);

				graphicsInfra->EndFrame();

				graphicsInfra->Present();
			}

			delete graphicsInfra;
			msDestroyNativeWindow(window);
		}

	};
}
