#ifndef PBR_H
#define PBR_H

Texture2D DiffuseTex : register(t0);
Texture2D NormalTex : register(t1);
Texture2D MetalnessTex : register(t2);
Texture2D LightMapTex : register(t3);
Texture2D SpecularTex : register(t4);
Texture2D BaseColorTex : register(t5);
Texture2D DiffuseRoughnessTex : register(t6);

/* from Frostbite engine */
struct PBRStandard
{
	// These all get packed into a GBuffer
	float3	worldNormal;
	uint	materialId;
	float3	baseColor;
	float	materialData;
	float	linearSmoothness;
	float	metalMask;
	float	reflectance;
	float	skyVisibility;
	float3  diffuseLighting;
};

void PackGbufferData(PBRStandard data, out float4 outGBuffer0, out float4 outGBuffer1, out float4 outGBuffer2)
{
	outGBuffer0 = float4(data.worldNormal, data.linearSmoothness);
	outGBuffer1 = float4(data.baseColor, data.skyVisibility);
	outGBuffer2 = float4(data.materialData, data.metalMask, data.reflectance, 0);
}

PBRStandard UnpackGbufferData(float4 gBuffer0, float4 gBuffer1, float4 gBuffer2)
{
    PBRStandard outData = (PBRStandard)0;
    outData.worldNormal = gBuffer0.xyz;
	outData.linearSmoothness = gBuffer0.w;
	outData.baseColor = gBuffer1.xyz;
    outData.skyVisibility = gBuffer1.w;
	outData.materialData = gBuffer2.x;
	outData.metalMask = gBuffer2.y;
	outData.reflectance = gBuffer2.z;
}

#endif
