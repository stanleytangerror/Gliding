#include "Includes.h"
#include "CppUnitTest.h"
#include "../Common/Serialization.h"
#include "../Common/GraphicsInfrastructure.h"
#include <Windows.h> // Added header

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
	TEST_CLASS(WindowsPlatformTest)
	{
	public:
		TEST_METHOD(CreateWindow_Succeed)
		{
#ifdef _DEBUG
			auto platformModule = LoadLibrary(L"WindowsPlatform_Debug_x64.dll");
#else
			auto platformModule = LoadLibrary(L"WindowsPlatform_Release_x64.dll");
#endif

			auto createNativeWindow = reinterpret_cast<Platform::CreateNativeWindow*>(GetProcAddress(platformModule, "CreateNativeWindow"));
			auto destroyNativeWindow = reinterpret_cast<Platform::DestroyNativeWindow*>(GetProcAddress(platformModule, "DestroyNativeWindow"));

			auto window = createNativeWindow(L"MainWindow", Vec2u{ 640, 360 });

			Sleep(1000);
			Assert::IsTrue(window->IsAlive());

			auto windowInfo = window->GetInfo();
			Assert::IsTrue(windowInfo.mNativeHandle != 0);
			Assert::AreEqual(windowInfo.mSize.x(), 640u);
			Assert::AreEqual(windowInfo.mSize.y(), 360u);

			destroyNativeWindow(window);
			Sleep(1000);
		}
	};
}
