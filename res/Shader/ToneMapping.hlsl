#include "Common.h"

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

float4 RtSize;

Texture2D SceneHdr;
SamplerState SamplerLinear;

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
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
	float3 sceneHdrColor = SceneHdr.Sample(SamplerLinear, uv);

	float3 sceneLdrColor = ACESFilm(sceneHdrColor * 0.01);
	output.color = float4(LinearToSrgb(sceneLdrColor), 1.0);

	return output;
}