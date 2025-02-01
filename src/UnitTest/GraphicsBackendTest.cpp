#include "Includes.h"
#include "CppUnitTest.h"
#include "../Common/Serialization.h"
#include "../Common/GraphicsInfrastructure.h"
#include <Windows.h> // Added header

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
	TEST_CLASS(GraphicsBackendTest)
	{
	public:
		TEST_METHOD(CreateDevice_Succeed)
		{
			auto graphicsInfra = CreateGraphicsInfra();
			Assert::IsTrue(graphicsInfra != nullptr);
			delete graphicsInfra;
		}

		TEST_METHOD(HelloTriangle_Succeed)
		{
			auto graphicsInfra = CreateGraphicsInfra();

			graphicsInfra->AdaptToWindow(windowInfo, frameCount);

			graphicsInfra->StartFrame();

			std::vector<Vec2f> points =
			{
				Vec2f{ 1.f, -1.f },
				Vec2f{ 1.f, 1.f },
				Vec2f{ -1.f, 1.f },
				Vec2f{ -1.f, -1.f }
			};
			std::vector<b8> vertices(points.size() * sizeof(Vec2f));
			std::copy(points.begin(), points.end(), reinterpret_cast<Vec2f*>(vertices.data()));

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

			std::vector<u16> ids = { 0, 1, 2, 0, 2, 3 };
			std::vector<b8> indices(ids.size() * sizeof(u16));
			std::copy(indices.begin(), indices.end(), reinterpret_cast<u16*>(ids.data()));
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

			auto rt = graphicsInfra->GetWindowBackBuffer(windowHandle);
			GI::RtvUsage rtv;
			rtv.mResource = rt;
			rtv.mDesc
				.SetFormat(GI::Format::FORMAT_R8G8B8A8_UNORM);

			GI::GraphicsPass pass;
			{
				pass.SetShader("HelloTriangle");

				pass.SetupDepthStencil()
					.SetDepthEnable(false)
					.SetStencilEnable(false);

				pass.SetRtv(0, rtv);
				pass.mViewPort.SetTopLeftX(targetOffset.x()).SetTopLeftY(targetOffset.y()).SetWidth(targetRect.x()).SetHeight(targetRect.y());
				pass.mScissorRect = { 0, 0, i32(data.targetSize.x()), i32(data.targetSize.y()) };

				pass.SetGeometry(vbv, 0, inputDescs, ibv, 0, ids.size());

				pass.AddCb4f("RtSize", Vec4f{ targetRect.x(), targetRect.y(), 1.f / targetRect.x(), 1.f / targetRect.y() });
			}

			graphicsInfra->GetRecorder()->AddGraphicsPass(pass);

			graphicsInfra->EndFrame();

			graphicsInfra->Present();

			delete graphicsInfra;
		}

	protected:
		static GI::IGraphicsInfra* CreateGraphicsInfra()
		{
#ifdef _DEBUG
			auto graphicsBackendModule = LoadLibrary(L"D3D12Backend_Debug_x64.dll");
#else
			auto graphicsBackendModule = LoadLibrary(L"D3D12Backend_Release_x64.dll");
#endif
			auto createInfraFunc = reinterpret_cast<GI::CreateGraphicsInfra*>(
				GetProcAddress(graphicsBackendModule, "CreateGraphicsInfra"));

			return createInfraFunc();
		}

		static GI::IGraphicsInfra* CreateWindow()
		{
#ifdef _DEBUG
			auto graphicsBackendModule = LoadLibrary(L"D3D12Backend_Debug_x64.dll");
#else
			auto graphicsBackendModule = LoadLibrary(L"D3D12Backend_Release_x64.dll");
#endif
			auto createInfraFunc = reinterpret_cast<GI::CreateGraphicsInfra*>(
				GetProcAddress(graphicsBackendModule, "CreateGraphicsInfra"));

			return createInfraFunc();
		}
	};
}
