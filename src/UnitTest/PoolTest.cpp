#include "Includes.h"
#include "CppUnitTest.h"
#include "../Common/Serialization.h"
#include "../Common/SuspendedRelease.h"
#include <ranges>
#include <algorithm>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace r = std::ranges;
namespace v = std::ranges::views;

namespace UnitTest
{
	TEST_CLASS(PoolTest)
	{
	public:
		TEST_METHOD(SuspendedReleasePool_Test)
		{
			enum Type { Active, Reset, Released };

			std::vector<Type*> ints;
			for (int i = 0; i < 1000; ++i)
			{
				ints.push_back(new Type());
			}
			int idx = 0;

			auto pool = new SuspendedReleasePool<Type>(
				[&idx, &ints]() { auto t = ints[idx++]; *t = Active; return t; },
				[](auto t) { *t = Reset; },
				[](auto t) { *t = Released; });

			for (int i = 0; i < 1000; ++i)
			{
				pool->AllocItem();
			}
			Assert::IsTrue(1000 == r::count_if(ints, [](Type* t) { return *t == Active; }));

			for (int i = 0; i < 500; ++i)
			{
				auto p = ints[i];
				pool->ScheduleReleaseItemAtTimestamp(1, p);
			}
			pool->UpdateTime(0);
			Assert::IsTrue(1000 == r::count_if(ints, [](Type* t) { return *t == Active; }));

			pool->UpdateTime(1);
			Assert::IsTrue(500 == r::count_if(ints, [](Type* t) { return *t == Active; }));
			Assert::IsTrue(500 == r::count_if(ints, [](Type* t) { return *t == Reset; }));

			for (int i = 500; i < 700; ++i)
			{
				auto p = ints[i];
				pool->ScheduleReleaseItemAtTimestamp(2, p);
			}
			pool->UpdateTime(2);
			Assert::IsTrue(300 == r::count_if(ints, [](Type* t) { return *t == Active; }));
			Assert::IsTrue(700 == r::count_if(ints, [](Type* t) { return *t == Reset; }));

			pool->ScheduleReleaseAllActiveItemsAtTimestamp(3);
			Assert::IsTrue(300 == r::count_if(ints, [](Type* t) { return *t == Active; }));
			Assert::IsTrue(700 == r::count_if(ints, [](Type* t) { return *t == Reset; }));

			pool->UpdateTime(3);
			Assert::IsTrue(1000 == r::count_if(ints, [](Type* t) { return *t == Reset; }));

			delete pool;
			Assert::IsTrue(1000 == r::count_if(ints, [](Type* t) { return *t == Released; }));
		}
	};
}
