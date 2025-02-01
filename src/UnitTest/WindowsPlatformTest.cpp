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
	protected:
		static inline HMODULE msPlatformModule;
		static inline Platform::CreateNativeWindow* msCreateNativeWindow = nullptr;
		static inline Platform::DestroyNativeWindow* msDestroyNativeWindow = nullptr;

		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
#ifdef _DEBUG
			msPlatformModule = LoadLibrary(L"WindowsPlatform_Debug_x64.dll");
#else
			msPlatformModule = LoadLibrary(L"WindowsPlatform_Release_x64.dll");
#endif
			msCreateNativeWindow = reinterpret_cast<Platform::CreateNativeWindow*>(GetProcAddress(msPlatformModule, "CreateNativeWindow"));
			msDestroyNativeWindow = reinterpret_cast<Platform::DestroyNativeWindow*>(GetProcAddress(msPlatformModule, "DestroyNativeWindow"));
		}

        TEST_CLASS_CLEANUP(ClassCleanup)
        {
            FreeLibrary(msPlatformModule);
        }

	public:
		TEST_METHOD(CreateWindow_Succeed)
		{
			auto window = msCreateNativeWindow(L"MainWindow", Vec2u{ 640, 360 });

			Sleep(1000);
			Assert::IsTrue(window->IsAlive());

			auto windowInfo = window->GetInfo();
			Assert::IsTrue(windowInfo.mNativeHandle != 0);
			Assert::AreEqual(windowInfo.mSize.x(), 640u);
			Assert::AreEqual(windowInfo.mSize.y(), 360u);

			msDestroyNativeWindow(window);
			Sleep(1000);
		}
	};
}
