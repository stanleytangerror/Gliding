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
	float4 worldPos : SV_POSITION;
	float3 worldTangent : TANGENT;
	float3 worldNormal : NORMAL;
	float3 worldBinormal : BINORMAL;
	float2 uv : TEXCOORD;
};

struct PSOutput
{
	float4 gBuffer0 : COLOR0;
	float4 gBuffer1 : COLOR1;
	float4 gBuffer2 : COLOR2;
};

cbuffer Param : register(b0)
{
	float4x4 viewMat;
	float4x4 projMat;
	float4x4 worldMat;
}

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.worldPos = mul(projMat, mul(viewMat, mul(worldMat, float4(vsin.position, 1))));
	result.uv = vsin.uv;
	result.worldNormal = mul((float3x3)worldMat, vsin.normal);
	result.worldBinormal = mul((float3x3)worldMat, vsin.binormal);
	result.worldTangent = mul((float3x3)worldMat, vsin.tangent);

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	const float4 baseColor = BaseColorTex.Sample(BaseColorTexSampler, input.uv).xyzw;
	clip(baseColor.w - 0.5);

	const float3x3 tbn = float3x3(input.worldTangent, input.worldBinormal, input.worldNormal);
	const float3 normalFromMap = normalize(NormalTex.Sample(NormalTexSampler, input.uv).xyz);
	const float3 worldNormal = normalize(mul(normalFromMap, tbn));

	const float4 metallicRoughness = MetalnessTex.Sample(MetalnessTexSampler, input.uv);
	const float metallic = metallicRoughness.x;
	const float roughness = metallicRoughness.y;

	PBRStandard matData = (PBRStandard)0;
	matData.worldNormal = worldNormal * 0.5 + 0.5;
	matData.baseColor = baseColor;
	matData.linearSmoothness = 1.0 - roughness;
	matData.reflectance = 0;
	matData.metalMask = metallic;

	PackGbufferData(matData, output.gBuffer0, output.gBuffer1, output.gBuffer2);

	return output;
}
