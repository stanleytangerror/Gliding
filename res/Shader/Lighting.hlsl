#include "Panorama.h"

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
	float4 color : COLOR;
};

Texture2D GBuffer0;
Texture2D GBuffer1;
Texture2D GBuffer2;
Texture2D PanoramicSky;
SamplerState SamplerLinearClamp;
SamplerState SamplerLinearWrap;

cbuffer Param : register(b0)
{
	float4 RtSize;
}

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = float4(vsin.position, 0.5, 1);

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	float2 uv = RtSize.zw * input.position.xy;
	const float3 normal = GBuffer1.Sample(SamplerLinearClamp, uv).xyz * 2.0 - 1.0;

	const float3 skyLight = SamplePanoramicSky(PanoramicSky, SamplerLinearWrap, normal);

	output.color = float4(skyLight, 1);

	return output;
}