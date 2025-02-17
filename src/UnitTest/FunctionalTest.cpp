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
		TEST_METHOD(Test)
		{
			auto p = std::make_unique<int>(1);

			auto f = MoveOnlyFunction<int(int, int)>(
				[p = std::move(p)](int a, int b)
			{
				return (*p) + a + b;
			}
			);

			Assert::AreEqual(6, f(2, 3));
		}
	};
}