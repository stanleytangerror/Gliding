#ifndef PANORAMA_H
#define PANORAMA_H

#include "Common.h"

float3 SamplePanoramicSky(Texture2D panoramicSkyTex, SamplerState samplerState, float3 dir, float blur)
{
	float theta = atan2(dir.x, dir.y);
	float phi = acos(dir.z);

	float2 uv = float2(theta * 0.5 * InvPI, phi * InvPI);
	return panoramicSkyTex.SampleLevel(samplerState, uv, blur).xyz;
}

#endif
