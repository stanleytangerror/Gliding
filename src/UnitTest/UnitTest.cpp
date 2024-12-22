#include "Includes.h"
#include "CppUnitTest.h"
#include "../Common/Serialization.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
	struct TypeA
	{
		f32 a;
		Vec2u b;
		Vec3f c;
		//std::array<u32, 5> d;
		//std::array<Vec3u, 5> d;
	};

	CLASS_REFLECTION(TypeA, a, b, c);

	TEST_CLASS(SerializationTest)
	{
	public:
		TEST_METHOD(BytesSerialization_Succeed)
		{
			TypeA a = {
				1.f, { 2, 3 }, { 4.1f, 4.2f, 4.3f },
				//{ 5, 6, 7, 8, 9 }
				//std::array<Vec3u, 5>{ Vec3u{5, 6, 7}, Vec3u{8, 9, 10} }
			};
			auto bytes = SerializeToBytes(a);
			auto b = DeserializeFromBytes<TypeA>(bytes);

			Assert::IsTrue(std::memcpy(&a, &b, sizeof(TypeA)));
		}
	};
}
