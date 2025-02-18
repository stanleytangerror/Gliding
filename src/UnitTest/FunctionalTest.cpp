#include "Includes.h"
#include "CppUnitTest.h"
#include "../Common/Serialization.h"
#include "../Common/Functional.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
	TEST_CLASS(FunctionalTest)
	{
	public:
		TEST_METHOD(CanCall)
		{
			auto p = std::make_unique<int>(1);
			auto f = MoveOnlyFunction<int(int, int)>(
				[p = std::move(p)](int a, int b)
			{
				return (*p) + a + b;
			});

			Assert::AreEqual(6, f(2, 3));
		}

		TEST_METHOD(CanMove)
		{
			auto p = std::make_unique<int>(1);
			auto f = MoveOnlyFunction<int(int, int)>([p = std::move(p)](int a, int b)
			{
				return (*p) + a + b;
			});
			auto f1 = std::move(f);
			auto r = f1(2, 3);
			Assert::AreEqual(6, r);
		}
	};
}