#include "Common.h"
#include "Panorama.h"
#include "Camera.h"

struct VSInput
{
	float2 position : POSITION;
};

struct PSInput
{
	float4 position : SV_POSITION;
};

struct PSOutput
{
	float4 color : COLOR0;
};

Texture2D PanoramicSky;
SamplerState PanoramicSkySampler;

float4 RtSize;

float3 CameraDir;

float4x4 InvViewMat;

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = float4(vsin.position, 0.5, 1);

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	float2 screenRatio = input.position.xy * RtSize.zw;
	float2 screenRatioFromCenter = (screenRatio * 2.0 - 1.0) * float2(1, -1);
	float2 offsetRadius = HalfFov * screenRatioFromCenter;
	float3 dirInViewSpace = normalize(float3(tan(offsetRadius), 1));
	float3 dirInWorldSpace = mul(InvViewMat, float4(dirInViewSpace, 0));

	const float3 skyLight = SamplePanoramicSky(PanoramicSky, PanoramicSkySampler, normalize(dirInWorldSpace));
	output.color = float4(skyLight * 50.0, 1);

	return output;
}
