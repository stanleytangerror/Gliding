#include "Common.h"
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

Texture2D LightViewDepth;
SamplerComparisonState LightViewDepthSampler;
Texture2D CameraViewDepth;
SamplerState CameraViewDepthSampler;

float4x4 CameraViewMat;
float4x4 CameraInvViewMat;
float4x4 CameraProjMat;
float4x4 CameraInvProjMat;

float4x4 LightViewMat;
float4x4 LightProjMat;

float4 RtSize;


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
	float deviceZ = CameraViewDepth.Sample(CameraViewDepthSampler, uv);
	const float3 worldSpacePos = GetWorldSpacePos(uv, deviceZ, float2(ViewSpaceNear, ViewSpaceFar), 
		CameraInvProjMat, CameraInvViewMat);

    float4 lightViewHomoPos = mul(LightProjMat, mul(LightViewMat, float4(worldSpacePos, 1)));
    float2 lightViewScreenUv = lightViewHomoPos.xy * 2 - 1;
    float lightViewDeviceZ = lightViewHomoPos.z / lightViewHomoPos.w;
	float shadowMask = LightViewDepth.SampleCmp(LightViewDepthSampler, lightViewScreenUv, lightViewDeviceZ);

    output.color = float4(1, 1, 1, 1) * shadowMask;
	return output;
}