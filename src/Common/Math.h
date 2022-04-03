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
template <typename T> using UniScaling = Eigen::UniformScaling<T>;
template <typename T> using Scaling = Eigen::DiagonalMatrix<T, 3>;

using Transformf = Transform<f32>;
using Translationf = Translation<f32>;
using Rotationf = Rotation<f32>;
using UniScalingf = UniScaling<f32>;
using Scalingf = Scaling<f32>;

namespace Math
{
	template <typename T>
	inline T Sqrt(T v) {}

	template <typename T>
	constexpr T Pi() { return T(3.14159265358979323846); }

	template <typename T>
	constexpr T HalfPi() { return Pi<T>() * 0.5; }

	template <typename T>
	constexpr T Align(T Val, u64 Alignment)
	{
		return (T)(((u64)Val + Alignment - 1) & ~(Alignment - 1));
	}

	template <typename T>
	constexpr float DegreeToRadian(const T deg) { return deg / 180.f * Pi<T>(); }

	template <typename T>
	constexpr float RadianToDegree(const T rad) { return rad * 180.f / Pi<T>(); }

	enum ValueCompareState
	{
		/* other state like not_equal is composed with or logic, e.g.,
		* greater or equal == greater_equal
		* greater or less == not_equal
		*/
		ValueCompareState_Equal = 0x0,
		ValueCompareState_Greater = 0x1,
		ValueCompareState_Less = 0x2
	};

	enum class Chirality
	{
		LeftHanded, RightHanded
	};

	enum GD_COMMON_API Axis3D
	{
		Axis3D_Xp,
		Axis3D_Xn,
		Axis3D_Yp,
		Axis3D_Yn,
		Axis3D_Zp,
		Axis3D_Zn
	};

	template <typename T>
	constexpr Vec3<T> Axis3DDir(const Axis3D& axis);

	GD_COMMON_API Mat33f GetRotation(Math::Axis3D source, Math::Axis3D target, Chirality chirality);

	struct GD_COMMON_API PerspectiveProjection
	{
		f32		mFovh = Math::DegreeToRadian(45.f);
		f32		mAspectRatio = 4.f / 3.f;
		f32		mNear = 1.f;
		f32		mFar = 1000.f;

		Mat44f	ComputeProjectionMatrix() const;
		f32		GetFarPlaneDepth() const;
		f32		GetNearPlaneDepth() const;
		ValueCompareState	GetNearerDepthCompare() const;
	};

	struct GD_COMMON_API OrthographicProjection
	{
		f32		mViewWidth = 100.f;
		f32		mViewHeight = 100.f;
		f32		mNear = 1.f;
		f32		mFar = 1000.f;

		Mat44f	ComputeProjectionMatrix() const;
	};

	template <typename T>
	struct GD_COMMON_API CameraTransform
	{
		/* In camera view space (right-handed, +z up), camera axis:
		 *		CamDir_v	+y,
		 *		CamUp_v		+z,
		 *		CamRight_v	+x
		 */

		Transform<T>	mWorldTransform = Transform<T>::Identity();

		Vec3<T>	CamDirInViewSpace() const { return Axis3DDir<T>(Axis3D_Yp); }
		Vec3<T>	CamUpInViewSpace() const { return Axis3DDir<T>(Axis3D_Zp); }
		Vec3<T>	CamRightInViewSpace() const { return Axis3DDir<T>(Axis3D_Xp); }

		Vec3<T>	CamDirInWorldSpace() const;
		Vec3<T>	CamUpInWorldSpace() const;
		Vec3<T>	CamRightInWorldSpace() const;
		Vec3<T>	CamPosInWorldSpace() const;

		Mat44<T> ComputeViewMatrix() const;
	};

	using CameraTransformf = CameraTransform<f32>;

	template <typename T, i32 Rols, i32 Cols>
	inline std::string ToString(const Mat<T, Rols, Cols>& mat);
}

#include "Math_inl.h"