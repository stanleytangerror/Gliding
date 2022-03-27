#include "PBR.h"

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float3 tangent : TANGENT;
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD;
};

struct PSOutput
{
	float4 gBuffer0 : COLOR0;
	float4 gBuffer1 : COLOR1;
	float4 gBuffer2 : COLOR2;
};

SamplerState SamplerLinear : register(s0);

cbuffer Param : register(b0)
{
	float4x4 viewMat;
	float4x4 projMat;
	float4x4 worldMat;
}

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	float3 pos = vsin.position;

	result.position = mul(projMat, mul(viewMat, mul(worldMat, float4(pos, 1))));
	result.normal = vsin.normal * 0.5 + 0.5;
	result.uv = vsin.uv;
	result.binormal = vsin.binormal;
	result.tangent = vsin.tangent;

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	const float4 baseColor = BaseColorTex.Sample(SamplerLinear, input.uv).xyzw;
	clip(baseColor.w - 0.5);

	const float3x3 tbn = float3x3(input.tangent, input.binormal, input.normal);

	const float2 normXy = NormalTex.Sample(SamplerLinear, input.uv).xy * 2 - 1;
	float3 normal = float3(normXy, sqrt(1 - dot(normXy, normXy)));
	normal = normalize(mul(normal, tbn));

	const float4 metallic = MetalnessTex.Sample(SamplerLinear, input.uv).x;

	const float roughness = DiffuseRoughnessTex.Sample(SamplerLinear, input.uv).w;

    const float4 spec = float4(0.5, 0.5, 0.5, 1.0);
    float3 specColor = spec.xyz;
	const float gloss = 1 - spec.w;

	PBRStandard matData = (PBRStandard)0;
	matData.worldNormal = normal;
	matData.baseColor = baseColor;
	matData.linearSmoothness = 1.0 - roughness;
	matData.reflectance = 0;
	matData.metalMask = metallic;

	PackGbufferData(matData, output.gBuffer0, output.gBuffer1, output.gBuffer2);

	return output;
}
