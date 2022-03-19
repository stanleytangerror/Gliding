#define PI (3.1415927)

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

float3 SamplePanoramicSky(Texture2D panoramicSkyTex, float3 dir)
{
	float theta = atan2(dir.x, dir.y);
	float phi = acos(dir.z);

	float2 uv = float2(theta / 2.0 / PI, phi / PI);
	return panoramicSkyTex.SampleLevel(SamplerLinearWrap, uv, 0).xyz;
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

	const float3 skyLight = SamplePanoramicSky(PanoramicSky, normal);

	output.color = float4(skyLight, 1);

	return output;
}