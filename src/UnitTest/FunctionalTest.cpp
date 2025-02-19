#include "Includes.h"
#include "CppUnitTest.h"
#include "../Common/Serialization.h"
#include "../Common/Functional.h"
#include <mutex>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
	template <typename T>
	struct Counter
	{
		static int InstanceCount;

		T member;
		bool memberValid = true;

		Counter() : member({}) { ++InstanceCount; }
		Counter(T v) : member(v) { ++InstanceCount; }
		Counter(const Counter& other) { ++InstanceCount; member = other.member; }
		Counter(Counter&& other) { ++InstanceCount; std::swap(member, other.member); other.memberValid = false; }
		Counter& operator=(const Counter& other) { member = other.member; }
		Counter& operator=(Counter&& other) { std::swap(member, other.member); other.memberValid = false; }
		~Counter() { --InstanceCount; }
	};

	template <typename T>
	int Counter<typename T>::InstanceCount = 0;

	struct MemoryAuditor
	{
		std::unordered_map<void*, std::size_t> allocations;
		mutable std::mutex mtx;

		void* Alloc(std::size_t size)
		{
			void* p = std::malloc(size);
			if (!p) throw std::bad_alloc();

			std::lock_guard<std::mutex> lock(mtx);
			allocations[p] = size;

			return p;
		}

		void Free(void* p) noexcept
		{
			std::lock_guard<std::mutex> lock(mtx);
			auto it = allocations.find(p);
			if (it != allocations.end())
			{
				allocations.erase(it);
			}
			std::free(p);
		}

		bool IsLeaking() const
		{
			std::lock_guard<std::mutex> lock(mtx);
			return !allocations.empty();
		}
	};

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

		TEST_METHOD(CountInstance)
		{
			auto p = std::make_unique<Counter<int>>(1);
			auto f = MoveOnlyFunction<int(int, int)>(
				[p = std::move(p)](int a, int b)
				{
					Assert::AreEqual(1, Counter<int>::InstanceCount);
					Assert::IsTrue(p->memberValid);
					return p->member + a + b;
			});
			Assert::IsFalse(p.operator bool());
			Assert::AreEqual(1, Counter<int>::InstanceCount);
			MoveOnlyFunction<int(int, int)> f1;
			f1 = std::move(f);
			Assert::IsFalse(f.operator bool());
			Assert::AreEqual(1, Counter<int>::InstanceCount);
			auto r = f1(2, 3);
			Assert::AreEqual(1, Counter<int>::InstanceCount);
			Assert::AreEqual(6, r);
		}

		static MemoryAuditor auditor;

		struct A
		{
			int i = 0;
			A(int i) : i(i) {}
			~A()
			{
				std::cout << "Dtor" << std::endl;
			}

			static void* operator new(std::size_t size)
			{
				return auditor.Alloc(size);
			}

			static void operator delete(void* p) noexcept
			{
				auditor.Free(p);
			}
		};

		TEST_METHOD(NoMemoryLeak)
		{
			{
				auto p = std::make_unique<A>(1);
				auto f = MoveOnlyFunction<int(int, int)>(
					[p = std::move(p)](int a, int b)
					{
						return p->i + a + b;
					});
				MoveOnlyFunction<int(int, int)> f1;
				f1 = std::move(f);
				auto r = f1(2, 3);
				Assert::AreEqual(6, r);
			}
			Assert::IsFalse(auditor.IsLeaking());
		}
	};

	MemoryAuditor FunctionalTest::auditor;
}