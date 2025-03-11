
struct VSInput
{
	float2 position : POSITION;
	float3 color : COLOR;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

cbuffer Param : register(b0)
{
	float4 rtSize;
}

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	float2 pos = (vsin.position + float2(0, -0.5)) * 0.5;
	float invAspectRatio = rtSize.y / rtSize.x;

	result.position = float4(pos * float2(invAspectRatio, 1.0), 0, 1);
	result.color = float4(vsin.color, 1);

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
}
