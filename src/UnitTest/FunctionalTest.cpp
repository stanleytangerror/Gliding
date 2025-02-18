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
		TEST_METHOD(SmallLambda_CanCall)
		{
			auto p = std::make_unique<int>(1);
			auto f = MoveOnlyFunction<int(int, int)>(
				[p = std::move(p)](int a, int b)
			{
				return (*p) + a + b;
			});

			Assert::AreEqual(6, f(2, 3));
		}

		TEST_METHOD(SmallLambda_CanMoveCtor)
		{
			auto p = std::make_unique<int>(1);
			auto f = MoveOnlyFunction<int(int, int)>(
				[p = std::move(p)](int a, int b)
			{
				return (*p) + a + b;
			});
			auto f1 = std::move(f);
			auto r = f1(2, 3);
			Assert::AreEqual(6, r);
		}

		TEST_METHOD(SmallLambda_CanMoveAssign)
		{
			auto p = std::make_unique<int>(1);
			auto f = MoveOnlyFunction<int(int, int)>(
				[p = std::move(p)](int a, int b)
			{
				return (*p) + a + b;
			});
			MoveOnlyFunction<int(int, int)> f1;
			f1 = std::move(f);
			auto r = f1(2, 3);
			Assert::AreEqual(6, r);
		}

		TEST_METHOD(LargeLambda_CanCall)
		{
			auto p = std::make_unique<int>(1);
			auto v = std::vector<std::string>(100, "test");
			auto f = MoveOnlyFunction<int(int, int)>(
				[p = std::move(p), v](int a, int b)
			{
				return (*p) + a + b;
			});

			Assert::AreEqual(6, f(2, 3));
		}

		TEST_METHOD(LargeLambda_CanMoveCtor)
		{
			auto p = std::make_unique<int>(1);
			auto v = std::vector<std::string>(100, "test");
			auto f = MoveOnlyFunction<int(int, int)>(
				[p = std::move(p), v](int a, int b)
			{
				return (*p) + a + b;
			});
			auto f1 = std::move(f);
			auto r = f1(2, 3);
			Assert::AreEqual(6, r);
		}

		TEST_METHOD(LargeLambda_CanMoveAssign)
		{
			auto p = std::make_unique<int>(1);
			auto v = std::vector<std::string>(100, "test");
			auto f = MoveOnlyFunction<int(int, int)>(
				[p = std::move(p), v](int a, int b)
			{
				return (*p) + a + b;
			});
			MoveOnlyFunction<int(int, int)> f1;
			f1 = std::move(f);
			auto r = f1(2, 3);
			Assert::AreEqual(6, r);
		}
	};
}