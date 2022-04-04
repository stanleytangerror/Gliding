#include "Panorama.h"
#include "PBR.h"
#include "Camera.h"

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
Texture2D SceneDepth;
SamplerState GBufferSampler;

Texture2D PanoramicSky;
SamplerState PanoramicSkySampler;

float4 RtSize;
float3 CameraDir;
float3 CameraPos;
float3 LightDir;
float3 LightColor;
float4x4 InvViewMat;

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = float4(vsin.position, 0.5, 1);

	return result;
}


PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	const float2 uv = RtSize.zw * input.position.xy;

	const float deviceZ = SceneDepth.Sample(GBufferSampler, uv).x;
	const float3 viewSpacePos = GetViewSpacePosFromDeviceZ(uv, deviceZ);
	const float3 worldSpacePos = mul(InvViewMat, float4(viewSpacePos, 1)).xyz;
	const float3 V = normalize(worldSpacePos - CameraPos);

	const float4 gBuffer0 = GBuffer0.Sample(GBufferSampler, uv);
	const float4 gBuffer1 = GBuffer1.Sample(GBufferSampler, uv);
	const float4 gBuffer2 = GBuffer2.Sample(GBufferSampler, uv);

	PBRStandard matData = UnpackGbufferData(gBuffer0, gBuffer1, gBuffer2);

	float3 diffuseColor = matData.baseColor * (1.0 - matData.metalMask);
	float3 specularColor = ComputeF0(matData.reflectance, matData.baseColor, matData.metalMask);

	FDirectLighting directLighting = DefaultLitBxDF(diffuseColor, specularColor, 1.0 - matData.linearSmoothness, matData.worldNormal, V, -LightDir, LightColor);

	output.color = float4(directLighting.Diffuse + directLighting.Specular, 1);

	return output;
}