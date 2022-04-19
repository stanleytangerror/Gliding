#ifndef PANORAMA_H
#define PANORAMA_H

#include "Common.h"

float2 DirToPanoramicUv(float3 dir)
{
	float theta = acos(dir.z);
	float phi = atan2(dir.x, dir.y);

	return float2(phi * 0.5 * InvPI, theta * InvPI);
}

float3 PanoramicUvToDir(float2 uv)
{
	float phi = PI * 2 * uv.x;
	float theta = PI * uv.y;

	float2 xy = float2(sin(phi), cos(phi)) * sin(theta);
	float z = cos(theta);
	return float3(xy, z);
}

float3 SamplePanoramicSky(Texture2D panoramicSkyTex, SamplerState samplerState, float3 dir, float blur)
{
	float2 uv = DirToPanoramicUv(dir);
	return panoramicSkyTex.SampleLevel(samplerState, uv, blur).xyz;
}

#endif
