#ifndef Camera_H
#define Camera_H

float4 FrustumInfo;
#define HalfFovHorizontal	(FrustumInfo.x)
#define HalfFovVertical		(FrustumInfo.y)
#define HalfFov         	(FrustumInfo.xy)
#define ViewSpaceNear       (FrustumInfo.z)
#define ViewSpaceFar        (FrustumInfo.w)

float GetViewDepthFromDeviceZ(float deviceZ)
{
	float Q = ViewSpaceFar / (ViewSpaceFar - ViewSpaceNear);
	return ViewSpaceNear / (1.0 - deviceZ / Q);
}

float3 GetViewSpacePosFromDeviceZ(float2 screenUv, float deviceZ)
{
	float viewSpaceDepth = GetViewDepthFromDeviceZ(deviceZ);
    float2 screenOffsetFromCenter = screenUv * 2.0 - 1.0;
    float3 unnormalizedDirInViewSpace = float3(tan(screenOffsetFromCenter * HalfFov), 1);
    
    return unnormalizedDirInViewSpace * viewSpaceDepth;
}

#endif