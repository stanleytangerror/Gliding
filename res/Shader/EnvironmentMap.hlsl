#include "Common.h"
#include "Panorama.h"

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

float4 RtSize;
float4 SemiSphereSampleInfo;
#define DELTA_RADIAN        (SemiSphereSampleInfo.x)
#define SAMPLE_COUNT        (SemiSphereSampleInfo.y)
#define INV_SAMPLE_COUNT    (SemiSphereSampleInfo.z)

Texture2D PanoramicSky;
SamplerState PanoramicSkySampler;

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = float4(vsin.position, 0.5, 1);

	return result;
}

float3x3 CreateTBNMatFromNormal(float3 normal)
{
    float3 defTangent = float3(1, 0, 0);
    
    float3 tangent = (abs(dot(normal, defTangent)) > 1.0 - EPS) ? float3(0, 1, 0) : defTangent;
    float3 biTangent = normalize(cross(normal, tangent));
    tangent = cross(biTangent, normal);

    return transpose(float3x3(tangent, biTangent, normal));
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	float2 uv = RtSize.zw * input.position.xy;
    float3 normalInWorldSpace = PanoramicUvToDir(uv);
    float3x3 tbn = CreateTBNMatFromNormal(normalInWorldSpace);

    float3 color = 0;
    for(float phi = 0.0; phi < 2.0 * PI - EPS; phi += DELTA_RADIAN)
    {
        for(float theta = 0; theta < (0.5 * PI - EPS); theta += DELTA_RADIAN)
        {
            float3 dirInLocalSpace = float3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            float3 dirInWorldSpace = mul(tbn, dirInLocalSpace);

            color += INV_SAMPLE_COUNT * cos(theta) * sin(theta) * SamplePanoramicSky(PanoramicSky, PanoramicSkySampler, dirInWorldSpace, 0);
        }
    }
    color = color * PI;
    output.color = float4(color, 1);

	return output;
}