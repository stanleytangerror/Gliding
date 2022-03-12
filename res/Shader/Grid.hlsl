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

cbuffer Param : register(b0)
{
	// float time;
}

Texture2D SceneHdr;
SamplerState SamplerLinear;

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = float4(vsin.position, 0, 0);

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	output.color = SceneHdr.Sample(SamplerLinear, input.position.xy);

	return output;
}