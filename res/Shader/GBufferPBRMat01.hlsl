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
	float3 worldPos : TEXCOORD1;
	float3 worldTangent : TANGENT;
	float3 worldNormal : NORMAL;
	float3 worldBinormal : BINORMAL;
	float2 uv : TEXCOORD0;
};

struct PSOutput
{
	float4 gBuffer0 : COLOR0;
	float4 gBuffer1 : COLOR1;
	float4 gBuffer2 : COLOR2;
};

float4x4 viewMat;
float4x4 projMat;
float4x4 worldMat;

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = mul(projMat, mul(viewMat, mul(worldMat, float4(vsin.position, 1))));
	result.worldPos = mul(worldMat, float4(vsin.position, 1)).xyz;
	result.uv = vsin.uv;
	result.worldNormal = normalize(mul((float3x3)worldMat, vsin.normal));
	result.worldBinormal = normalize(mul((float3x3)worldMat, vsin.binormal));
	result.worldTangent = normalize(mul((float3x3)worldMat, vsin.tangent));

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	float2 uv = input.uv;
	uv.y = 1.0 - uv.y; // gl texture uv

	const float4 baseColor = GetBaseColorValue(uv);
	clip(baseColor.w - 0.5);

	const float3 normalFromMap = GetNormalValue(uv).xyz * 2.0 - 1.0;
	// float3x3 take 3 float3 as rows
	const float3x3 tbn = transpose(float3x3(
		normalize(input.worldTangent), 
		normalize(input.worldBinormal), 
		normalize(input.worldNormal)));
	const float3 worldNormal = normalize(mul(tbn, normalFromMap));

	const float roughness = GetRoughnessValue(uv).y;
	const float metallic = GetMetallicValue(uv).z;

	PBRStandard matData = (PBRStandard)0;
	matData.worldNormal = worldNormal;
	matData.baseColor = baseColor;
	matData.linearSmoothness = 1.0 - roughness;
	matData.reflectance = 0.5;
	matData.metalMask = metallic;

	PackGbufferData(matData, output.gBuffer0, output.gBuffer1, output.gBuffer2);

	return output;
}
