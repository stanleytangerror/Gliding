#ifndef Camera_H
#define Camera_H

float4 FrustumInfo;
#define HalfFovHorizontal	(FrustumInfo.x)
#define HalfFovVertical		(FrustumInfo.y)
#define HalfFov         	(FrustumInfo.xy)
#define ViewSpaceNear       (FrustumInfo.z)
#define ViewSpaceFar        (FrustumInfo.w)

float3 GetNDCSpacePos(float2 screenUv, float deviceZ)
{
    return float3(screenUv * 2.0 - 1.0, deviceZ);
}

float GetClipDivideW(float deviceZ, float2 viewSpaceDepthRange)
{
    float N = viewSpaceDepthRange.x;
    float F = viewSpaceDepthRange.y;
	float Q = F / (F - N);
	return N / (1.0 - deviceZ / Q);
}

float4 GetHomoSpacePos(float2 screenUv, float deviceZ, float2 viewSpaceDepthRange)
{
    float3 ndcPos = GetNDCSpacePos(screenUv, deviceZ);
    float w = GetClipDivideW(deviceZ, viewSpaceDepthRange);
    return float4(ndcPos, 1) * w;
}

float3 GetViewSpacePos(float2 screenUv, float deviceZ, float2 viewSpaceDepthRange, float4x4 invProjMat)
{
    return mul(invProjMat, GetHomoSpacePos(screenUv, deviceZ, viewSpaceDepthRange)).xyz;
}

float3 GetWorldSpacePos(float2 screenUv, float deviceZ, float2 viewSpaceDepthRange, float4x4 invProjMat, float4x4 invViewMat)
{
    float3 viewSpacePos = GetViewSpacePos(screenUv, deviceZ, viewSpaceDepthRange, invProjMat);
    return mul(invViewMat, float4(viewSpacePos, 1)).xyz;
}

#endif