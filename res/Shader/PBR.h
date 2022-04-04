#ifndef PBR_H
#define PBR_H

#include "Common.h"

#define DeclareMaterialTexture(name) \
Texture2D name##Tex;\
SamplerState name##TexSampler;

DeclareMaterialTexture(Diffuse);
DeclareMaterialTexture(Normal);
DeclareMaterialTexture(Metalness);
DeclareMaterialTexture(LightMap);
DeclareMaterialTexture(Specular);
DeclareMaterialTexture(BaseColor);
DeclareMaterialTexture(DiffuseRoughness);


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
    return outData;
}

/* from UE4 */
struct BxDFContext
{
	float NoV;
	float NoL;
	float VoL;
	float NoH;
	float VoH;
};

void Init( inout BxDFContext Context,  float3  N,  float3  V,  float3  L )
{
	Context.NoL = dot(N, L);
	Context.NoV = dot(N, V);
	Context.VoL = dot(V, L);
	float InvLenH = rsqrt( 2 + 2 * Context.VoL );
	Context.NoH = saturate( ( Context.NoL + Context.NoV ) * InvLenH );
	Context.VoH = saturate( InvLenH + InvLenH * Context.VoL );

	Context.NoL = saturate(Context.NoL);
	Context.VoL = saturate(Context.VoL);
}

float DielectricSpecularToF0(float Specular)
{
	return 0.08f * Specular;
}

float DielectricF0ToIor(float F0)
{
	return 2.0f / (1.0f - sqrt(F0)) - 1.0f;
}

float3 ComputeF0(float Specular, float3 BaseColor, float Metallic)
{
	return lerp(DielectricSpecularToF0(Specular).xxx, BaseColor, Metallic.xxx);
}

float3 Diffuse_Lambert( float3 DiffuseColor )
{
	return DiffuseColor * (1 / PI);
}

float D_GGX( float a2, float NoH )
{
	float d = ( NoH * a2 - NoH ) * NoH + 1;
	return a2 / ( PI*d*d );
}

float Vis_SmithJointApprox( float a2, float NoV, float NoL )
{
	float a = sqrt(a2);
	float Vis_SmithV = NoL * ( NoV * ( 1 - a ) + a );
	float Vis_SmithL = NoV * ( NoL * ( 1 - a ) + a );
	return 0.5 * rcp( Vis_SmithV + Vis_SmithL );
}

float3 F_Schlick( float3 SpecularColor, float VoH )
{
	float Fc = Pow5( 1 - VoH );

	return saturate( 50.0 * SpecularColor.g ) * Fc + (1 - Fc) * SpecularColor;
}

float3 SpecularGGX(float Roughness, float3 SpecularColor, BxDFContext Context, float NoL)
{
	float a2 = Pow4( Roughness );
	float Energy = 1.0; //EnergyNormalization( a2, Context.VoH, AreaLight );

	float D = D_GGX( a2, Context.NoH ) * Energy;
	float Vis = Vis_SmithJointApprox( a2, Context.NoV, NoL );
	float3 F = F_Schlick( SpecularColor, Context.VoH );

	return (D * Vis) * F;
}

struct FDirectLighting
{
	float3 Diffuse;
	float3 Specular;
	float3 Transmission;
};

FDirectLighting DefaultLitBxDF(float3 diffuseColor, float3 specularColor, float roughness, float3 N, float3 V, float3 L, float3 lightColor)
{
	BxDFContext Context;
	Init( Context, N, V, L );
	Context.NoV = saturate( abs( Context.NoV ) + 1e-5 );

	FDirectLighting Lighting;
	Lighting.Diffuse = (lightColor * Context.NoL) * Diffuse_Lambert(diffuseColor);
	Lighting.Specular = (lightColor * Context.NoL) * SpecularGGX(roughness, specularColor, Context, Context.NoL);
	Lighting.Transmission = 0;

	return Lighting;
}

#endif
