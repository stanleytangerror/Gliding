#include "Common.h"

struct VSInput // map to ImDrawVert
{
	float2 position : POSITION;
    float2 uv : TEXCOORD;
    uint color : COLOR;
};

struct PSInput
{
	float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct PSOutput
{
	float4 color : COLOR;
};

float4x4 WvpMat;

#if USE_TEXTURE
    Texture2D SourceTex;
    SamplerState SourceTexSampler;
#endif

PSInput VSMain(VSInput vsin)
{
	PSInput result;
	result.position = mul(WvpMat, float4(vsin.position, 0.5, 1));
    result.uv = vsin.uv;
    result.color = UnpackColorABGRU32(vsin.color);
	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

    float4 color = input.color;
    #if USE_TEXTURE
        color *= SourceTex.Sample(SourceTexSampler, input.uv);
    #endif

    output.color = color;
	return output;
}
