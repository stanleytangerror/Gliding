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

Texture2D InputTex0;
Texture2D InputTex1;
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
	const float3 color0 = InputTex0.Sample(SamplerLinear, uv).xyz;
	const float3 color1 = InputTex1.Sample(SamplerLinear, uv).xyz;
	
	output.color = float4(color0 * color1, 1);

	return output;
}