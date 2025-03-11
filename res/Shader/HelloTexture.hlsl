
struct VSInput
{
	float2 position : POSITION;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer Param : register(b0)
{
	float4 rtSize;
}

Texture2D inputTex : register(t0);
SamplerState inputTexSampler : register(s0);

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	float2 pos = (vsin.position - 0.5) * 0.5;
	float invAspectRatio = rtSize.y / rtSize.x;

	result.position = float4(pos * float2(invAspectRatio, 1.0), 0, 1);
	result.uv = vsin.position;

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return inputTex.Sample(inputTexSampler, input.uv);
}
