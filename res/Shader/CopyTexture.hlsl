#include "Panorama.h"
#include "PBR.h"
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
	float4 color : COLOR;
};

float4 RtSize;

Texture2D SourceTex;
SamplerState SourceTexSampler;

PSInput VSMain(VSInput vsin)
{
	PSInput result;
	result.position = float4(vsin.position, 0.5, 1);
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;
	const float2 uv = RtSize.zw * input.position.xy;
	const float3 color = SourceTex.Sample(SourceTexSampler, uv).xyz;
    output.color = float4(color, 1);
	return output;
}