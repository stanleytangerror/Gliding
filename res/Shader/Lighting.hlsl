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

Texture2D ShadowMask;
SamplerState ShadowMaskSampler;

Texture2D IrradianceMap;
SamplerState IrradianceMapSampler;

Texture2D PanoramicSky;
SamplerState PanoramicSkySampler;
float SkyLightIntensity;

Texture2D BRDFIntegrationMap;
SamplerState BRDFIntegrationMapSampler;

float4 RtSize;
float3 CameraDir;
float3 CameraPos;
float3 LightDir;
float3 LightColor;
float4x4 InvViewMat;
float4x4 InvProjMat;

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = float4(vsin.position, 0.5, 1);

	return result;
}

float GetMipLevelFromRoughness(float roughness)
{
	return 6;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	const float2 uv = RtSize.zw * input.position.xy;

	const float deviceZ = SceneDepth.Sample(GBufferSampler, uv).x;
	const float3 worldSpacePos = GetWorldSpacePos(uv, deviceZ, float2(ViewSpaceNear, ViewSpaceFar), 
		InvProjMat, InvViewMat);

	const float3 V = normalize(CameraPos - worldSpacePos);

	const float4 gBuffer0 = GBuffer0.Sample(GBufferSampler, uv);
	const float4 gBuffer1 = GBuffer1.Sample(GBufferSampler, uv);
	const float4 gBuffer2 = GBuffer2.Sample(GBufferSampler, uv);

	const float shadowMask = ShadowMask.Sample(ShadowMaskSampler, uv).x;

	PBRStandard matData = UnpackGbufferData(gBuffer0, gBuffer1, gBuffer2);
	
	float3 diffuseColor = matData.baseColor * (1.0 - matData.metalMask);
	float3 specularColor = ComputeF0(matData.reflectance, matData.baseColor, matData.metalMask);

	FDirectLighting directLighting = DefaultLitBxDF(diffuseColor, specularColor, 1.0 - matData.linearSmoothness, matData.worldNormal, V, -LightDir, LightColor, shadowMask);

	float3 indirectDiffuse = float3(0, 0, 0);
	float3 indirectSpecular = float3(0, 0, 0);
	{
		float NoV = saturate(dot(V, matData.worldNormal));
		float3 irradiance = SamplePanoramicSky(IrradianceMap, IrradianceMapSampler, matData.worldNormal, 0);
		indirectDiffuse = (1.0 - F_Schlick(specularColor, NoV)) * irradiance * diffuseColor;

		const float3 reflectDir = reflect(LightDir, matData.worldNormal);
		const float lod = GetMipLevelFromRoughness(1.0 - matData.linearSmoothness);
		const float3 prefilteredColor = SamplePanoramicSky(PanoramicSky, PanoramicSkySampler, reflectDir, lod) * SkyLightIntensity;

		const float roughness = 1.0 - matData.linearSmoothness;
		const float NdotV = dot(matData.worldNormal, V);
		const float2 envBRDF = BRDFIntegrationMap.Sample(BRDFIntegrationMapSampler, float2(roughness, NdotV)).xy;
		indirectSpecular = prefilteredColor * (specularColor * envBRDF.x + envBRDF.y);
	}

	output.color = float4(directLighting.Diffuse + directLighting.Specular + indirectDiffuse + indirectSpecular, 1);

	return output;
}