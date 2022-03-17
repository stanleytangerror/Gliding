#pragma once

#include "CommonTypes.h"
#include <array>
#include <Eigen\Eigen>

template <typename T> using Vec2 = Eigen::Vector2<T>;
template <typename T> using Vec3 = Eigen::Vector3<T>;
template <typename T> using Vec4 = Eigen::Vector4<T>;
template <typename T> using Quat = Eigen::Quaternion<T>;

using Vec2i = Vec2<i32>;
using Vec2f = Vec2<f32>;
using Vec3f = Vec3<f32>;
using Vec3i = Vec3<i32>;
using Vec4f = Vec4<f32>;

// use column-major storage, independent of computation logic
template <typename T, i32 Rols, i32 Cols> using Mat = Eigen::Matrix<T, Rols, Cols, Eigen::ColMajor>;
template <typename T> using Mat33 = Mat<T, 3, 3>;
template <typename T> using Mat44 = Mat<T, 4, 4>;

using Mat33f = Mat33<f32>;
using Mat44f = Mat44<f32>;

template <typename T> using Transform = Eigen::Transform<T, 3, Eigen::Isometry, Eigen::ColMajor>;
template <typename T> using Translation = Eigen::Translation<T, 3>;
template <typename T> using Rotation = Eigen::AngleAxis<T>;

using Transformf = Transform<f32>;
using Translationf = Translation<f32>;
using Rotationf = Rotation<f32>;

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

	template <typename T, i32 Rols, i32 Cols>
	inline std::string ToString(const Mat<T, Rols, Cols>& mat);
}

#include "Math_inl.h"