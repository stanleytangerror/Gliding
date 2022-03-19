#define PI (3.1415927)

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	// float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD;
};

struct PSOutput
{
	float4 color : COLOR0;
};

Texture2D PanoramicSky;
SamplerState SamplerLinearClamp;
SamplerState SamplerLinearWrap;

cbuffer Param : register(b0)
{
	float4x4 viewMat;
	float4x4 projMat;
	float4x4 worldMat;
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

	float3 pos = vsin.position;

	result.position = mul(projMat, mul(viewMat, mul(worldMat, float4(pos, 1))));
	result.worldPos = mul(worldMat, float4(pos, 1)).xyz;

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	const float3 skyLight = SamplePanoramicSky(PanoramicSky, normalize(input.worldPos));
	output.color = float4(skyLight, 1);

	return output;
}
