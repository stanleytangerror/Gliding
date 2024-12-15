#include "Common/CommonPch.h"
#include "ModelProcess.h"

namespace ModelProcess
{
	float GetScalarParam(const Channel& channel, const char* name, float defaultValue)
	{
		for (const auto& param : channel.ScalarParams)
		{
			if (param.first == name)
			{
				return param.second;
			}
		}
		return defaultValue;
	}

	Vec2f GetVec2fParam(const Channel& channel, const char* name, const Vec2f& defaultValue)
	{
		for (const auto& param : channel.Vector2Params)
		{
			if (param.first == name)
			{
				return param.second;
			}
		}
		return defaultValue;
	}

	Vec3f GetVec3fParam(const Channel& channel, const char* name, const Vec3f& defaultValue)
	{
		for (const auto& param : channel.Vector3Params)
		{
			if (param.first == name)
			{
				return param.second;
			}
		}
		return defaultValue;
	}

	Vec4f GetVec4fParam(const Channel& channel, const char* name, const Vec4f& defaultValue)
	{
		for (const auto& param : channel.Vector4Params)
		{
			if (param.first == name)
			{
				return param.second;
			}
		}
		return defaultValue;
	}
}