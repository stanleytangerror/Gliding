#pragma once

#include "CommonTypes.h"
#include <array>
#include <algorithm>
#include <utility>
#include <Eigen\Eigen>

template <typename T> using Vec2 = Eigen::Vector2<T>;
template <typename T> using Vec3 = Eigen::Vector3<T>;
template <typename T> using Vec4 = Eigen::Vector4<T>;
template <typename T> using Quat = Eigen::Quaternion<T>;

using Vec2i = Vec2<i32>;
using Vec2u = Vec2<u32>;
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
template <typename T> using Rotation = Eigen::Quaternion<T>;
template <typename T> using AngleAxis = Eigen::AngleAxis<T>;
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
	constexpr T HalfPi() { return Pi<T>() * T(0.5); }

	template <typename T>
	constexpr T Epsilon() { return T(1e-5); }

	template <typename T>
	constexpr T Align(T Val, u64 Alignment)
	{
		return (T)(((u64)Val + Alignment - 1) & ~(Alignment - 1));
	}

	template <typename T>
	constexpr bool AlmostEqual(T a, T b) { return std::max(a, b) <= std::min(a, b) + Epsilon<T>(); }

	template <typename T, i32 Rols, i32 Cols>
	inline bool AlmostEqual(const Mat<T, Rols, Cols>& a, const Mat<T, Rols, Cols>& b);

	template <typename T>
	constexpr bool AlmostZero(T a) { return AlmostEqual<T>(a, T(0)); }

	template <typename T, i32 Rols, i32 Cols>
	constexpr bool AlmostZero(const Mat<T, Rols, Cols>& a) { return AlmostEqual<T, Rols, Cols>(a, Mat<T, Rols, Cols>::Zero()); }

	template <typename T>
	constexpr float DegreeToRadian(const T deg) { return deg / T(180) * Pi<T>(); }

	template <typename T>
	constexpr float RadianToDegree(const T rad) { return rad * T(180) / Pi<T>(); }

	template <typename T>
	inline Rotationf FromAngleAxis(T angleInRad, const Vec3<T>& axis) { return Rotation<T>(AngleAxis<T>(angleInRad, axis)); }

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

	template <typename T>
	Mat33<T> GetRotation(Math::Axis3D source, Math::Axis3D target, Chirality chirality);

	template <typename T>
	struct PerspectiveProjection
	{
		T		mFovHorizontal = Math::DegreeToRadian<T>(45);
		T		mAspectRatio = T(16) / T(9);
		T		mNear = T(1);
		T		mFar = T(1000);

		Mat44<T>	ComputeProjectionMatrix() const;
		Mat44<T>	ComputeInvProjectionMatrix() const;

		T			GetFarPlaneDeviceDepth() const;
		T			GetNearPlaneDeviceDepth() const;
		T			GetHalfFovHorizontal() const;
		T			GetHalfFovVertical() const;
		ValueCompareState	GetNearerDepthCompare() const;
	};

	using PerspectiveProjectionf = PerspectiveProjection<f32>;

	template <typename T>
	struct OrthographicProjection
	{
		T		mViewWidth = T(100);
		T		mViewHeight = T(100);
		T		mNear = T(1);
		T		mFar = T(1000);

		Mat44<T>	ComputeProjectionMatrix() const;
		T			GetFarPlaneDeviceDepth() const;
		T			GetNearPlaneDeviceDepth() const;
		ValueCompareState	GetNearerDepthCompare() const;
	};

	using OrthographicProjectionf = OrthographicProjection<f32>;

	template <typename T>
	struct CameraTransform
	{
		/* In camera view space (right-handed, +z up), camera axis:
		 *		CamDir_v	+y,
		 *		CamUp_v		+z,
		 *		CamRight_v	+x
		 *
		 * By default camera axis in world space (right-handed, +z up, default no world rotation):
		 * (NOTE: it is coincidental that view space axis aligned the same with world space axis)
		 *		CamDir_w	+y,
		 *		CamUp_w		+z,
		 *		CamRight_w	+x
		 */

		/* WorldTransform = WorldTransform * WorldRotation (apply rotation first) */
		Rotation<T>	mWorldRotation = Rotation<T>::Identity();
		Translation<T>	mWorldTranslation = Translation<T>::Identity();

		void AlignCamera(const Vec3<T>& dirWorld, const Vec3<T>& upWorld, const Vec3<T>& rightWorld);
		void MoveCamera(const Vec3<T>& posWorld);

		Vec3<T>	CamDirInViewSpace() const { return Axis3DDir<T>(Axis3D_Yp); }
		Vec3<T>	CamUpInViewSpace() const { return Axis3DDir<T>(Axis3D_Zp); }
		Vec3<T>	CamRightInViewSpace() const { return Axis3DDir<T>(Axis3D_Xp); }

		Vec3<T>	CamDirInWorldSpace() const;
		Vec3<T>	CamUpInWorldSpace() const;
		Vec3<T>	CamRightInWorldSpace() const;
		Vec3<T>	CamPosInWorldSpace() const;

		Mat44<T> ComputeViewMatrix() const;
		Mat44<T> ComputeInvViewMatrix() const;
	};

	using CameraTransformf = CameraTransform<f32>;

	template <typename T, i32 Rols, i32 Cols>
	inline std::string ToString(const Mat<T, Rols, Cols>& mat);
}

#include "Math_inl.h"