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

Texture2D SourceTex;
SamplerState SourceTexSampler;
float4 BlurTargetSize;

float4 Weights[WEIGHT_SIZE];

PSInput VSMain(VSInput vsin)
{
	PSInput result;
	result.position = float4(vsin.position, 0.5, 1);
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	float2 uv = BlurTargetSize.zw * input.position.xy;

    float4 color = SourceTex.Sample(SourceTexSampler, uv) * Weights[0][0];
    for (int i = 1; i < WEIGHT_SIZE * 4; ++i)
    {
        #if HORIZONTAL
            float2 uvOffset = BlurTargetSize.zw * float2(i, 0);
        #elif VERTICAL
            float2 uvOffset = BlurTargetSize.zw * float2(0, i);
        #endif
        float weight = Weights[i / 4][i % 4];
        color += SourceTex.Sample(SourceTexSampler, uv + uvOffset) * weight;
        color += SourceTex.Sample(SourceTexSampler, uv - uvOffset) * weight;
    }

    output.color = color;

	return output;
}