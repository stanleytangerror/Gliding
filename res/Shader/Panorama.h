#ifndef PANORAMA_H
#define PANORAMA_H

#include "Common.h"

float3 SamplePanoramicSky(Texture2D panoramicSkyTex, SamplerState samplerState, float3 dir)
{
	float theta = atan2(dir.x, dir.y);
	float phi = acos(dir.z);

	float2 uv = float2(theta / 2.0 / PI, phi / PI);
	return panoramicSkyTex.SampleLevel(samplerState, uv, 0).xyz;
}

#endif
