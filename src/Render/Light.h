#pragma once

struct GD_RENDER_API DirectionalLight
{
	Vec3f	mLightColor = Vec3f::Ones();
	f32		mLightIntensity = 0.f;

	Math::CameraTransformf			mWorldTransform;
	Math::OrthographicProjection	mLightViewProj;
};