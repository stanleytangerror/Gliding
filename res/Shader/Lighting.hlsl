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
SamplerState SamplerLinear;

cbuffer Param : register(b0)
{
	float4 RtSize;
}

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = float4(vsin.position, 0, 0);

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	float2 uv = RtSize.zw * input.position.xy;
	const float3 color0 = GBuffer1.Sample(SamplerLinear, uv).xyz;
	
	output.color = float4(color0, 1);

	return output;
}