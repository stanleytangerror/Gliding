#pragma once

#include "CommonTypes.h"
#include <array>

template <typename T, i32 Row>
struct Vec
{
	std::array<T, Row> m = {};

	T			x() const { return m[0]; }
	T			y() const { return m[1]; }
	T			z() const { return m[2]; }
	T			w() const { return m[3]; }

	T			operator[](i32 i) const { return m[i]; }
	T&			operator[](i32 i) { return m[i]; }
	Vec<T, Row>	operator-() const;

	Vec<T, Row>	GetNormalized() const;
	T			GetLength() const;
};

template <typename T> using Vec2 = Vec<T, 2>;
template <typename T> using Vec3 = Vec<T, 3>;
template <typename T> using Vec4 = Vec<T, 4>;

template <typename T>
struct Quat
{
	std::array<T, 4> m = {};

	T			x() const { return m[0]; }
	T			y() const { return m[1]; }
	T			z() const { return m[2]; }
	T			w() const { return m[3]; }
};

using Vec2f = Vec2<f32>;
using Vec3f = Vec3<f32>;
using Vec3i = Vec3<i32>;
using Vec4f = Vec4<f32>;

template <typename T, i32 Row, i32 Col>
struct Mat
{
	std::array<Vec<T, Row>, Col> mColumns = {};

	void	SetRow(i32 r, const Vec<T, Col>& v);
	void	SetCol(i32 c, const Vec<T, Row>& v);

	template <i32 SubCol>
	void	SetRow(i32 r, const Vec<T, SubCol>& v);
	
	template <i32 SubRow>
	void	SetCol(i32 c, const Vec<T, SubRow>& v);

	T		Get(i32 r, i32 c) const { return mColumns[c][r]; }
	T&		Get(i32 r, i32 c) { return mColumns[c][r]; }

	static Mat<T, Row, Col> GetIdentity();
};

template <typename T> using Mat33 = Mat<T, 3, 3>;
template <typename T> using Mat44 = Mat<T, 4, 4>;

using Mat33f = Mat33<f32>;
using Mat44f = Mat44<f32>;

namespace Math
{
	template <typename T>
	inline T Sqrt(T v) {}

	template <typename T>
	constexpr T Pi() { return T(3.14159265358979323846); }

	template <typename T>
	constexpr T Align(T Val, u64 Alignment)
	{
		return (T)(((u64)Val + Alignment - 1) & ~(Alignment - 1));
	}

	template <typename T>
	constexpr float DegreeToRadian(const T deg) { return deg / 180.f * Pi<T>(); }

	template <typename T>
	constexpr float RadianToDegree(const T rad) { return rad * 180.f / Pi<T>(); }

	struct GD_COMMON_API PerspectiveProjection
	{
		f32		mFovh = Math::DegreeToRadian(45.f);
		f32		mAspectRatio = 4.f / 3.f;
		f32		mNear = 1.f;
		f32		mFar = 1000.f;

		Mat44f	ComputeProjectionMatrix();
	};

	struct GD_COMMON_API OrthographicProjection
	{
		f32		mViewWidth = 100.f;
		f32		mViewHeight = 100.f;
		f32		mNear = 1.f;
		f32		mFar = 1000.f;

		Mat44f	ComputeProjectionMatrix();
	};

	struct ViewMatrix
	{
		Vec3f	mPos = {};
		Vec3f	mDir = {};
		Vec3f	mRight = {};
		Vec3f	mUp = {};
	};

	template <typename T>
	inline Mat44<T> ComputeViewMatrix(const Vec3<T>& pos, const Vec3<T>& dir, const Vec3<T>& up, const Vec3<T>& right);
	
	template <typename T>
	inline Mat44<T> ComputeModelMatrix(const Vec3<T>& pos, const Quat<T>& rot, const Vec3<T>& scale);
}

#include "Math_inl.h"