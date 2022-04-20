#include "Common.h"
#include "Panorama.h"
#include "PBR.h"

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

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = float4(vsin.position, 0.5, 1);

	return result;
}

float4 RtSize;

#if GENERATE_IRRADIANCE_MAP

    float4 SemiSphereSampleInfo;
    #define DELTA_RADIAN        (SemiSphereSampleInfo.x)
    #define SAMPLE_COUNT        (SemiSphereSampleInfo.y)
    #define INV_SAMPLE_COUNT    (SemiSphereSampleInfo.z)

    Texture2D PanoramicSky;
    SamplerState PanoramicSkySampler;

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

#elif GENERATE_INTEGRATE_BRDF

    float RadicalInverse_VdC(uint bits) 
    {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10; // / 0x100000000
    }

    float2 Hammersley(uint i, uint N)
    {
        return float2(float(i)/float(N), RadicalInverse_VdC(i));
    }  

    float3 ImportanceSampleGGX( float2 E, float a2 )
    {
        float Phi = 2 * PI * E.x;
        float CosTheta = sqrt( (1 - E.y) / ( 1 + (a2 - 1) * E.y ) );
        float SinTheta = sqrt( 1 - CosTheta * CosTheta );

        float3 H;
        H.x = SinTheta * cos( Phi );
        H.y = SinTheta * sin( Phi );
        H.z = CosTheta;
        
        float d = ( CosTheta * a2 - CosTheta ) * CosTheta + 1;
        float D = a2 / ( PI*d*d );
        float PDF = D * CosTheta;

        return float4( H, PDF );
    }

    float2 IntegrateBRDF(float Roughness, float NoV)
    {
        float3 V;
        V.x = sqrt( 1.0f - NoV * NoV );	// sin
        V.y = 0;
        V.z = NoV;						// cos

        float A = 0;
        float B = 0;
        float C = 0;

        const uint NumSamples = 64;
        for( uint i = 0; i < NumSamples; i++ )
        {
            float2 E = Hammersley(i, NumSamples);

            float3 H = ImportanceSampleGGX( E, Pow4(Roughness) ).xyz;
            float3 L = 2 * dot( V, H ) * H - V;

            float NoL = saturate( L.z );
            float NoH = saturate( H.z );
            float VoH = saturate( dot( V, H ) );

            if( NoL > 0 )
            {
                float a = Roughness * Roughness;
                float a2 = a*a;
                float Vis = Vis_SmithJointApprox( a2, NoV, NoL );
                float Vis_SmithV = NoL * sqrt( NoV * (NoV - NoV * a2) + a2 );
                float Vis_SmithL = NoV * sqrt( NoL * (NoL - NoL * a2) + a2 );
                float NoL_Vis_PDF = NoL * Vis * (4 * VoH / NoH);

                float Fc = pow( 1 - VoH, 5 );
                A += (1 - Fc) * NoL_Vis_PDF;
                B += Fc * NoL_Vis_PDF;
            }
        }

        return float2(A, B) / NumSamples;
    }

    PSOutput PSMain(PSInput input) : SV_TARGET
    {
        PSOutput output;
        
        float2 uv = RtSize.zw * input.position.xy;
        float2 envBrdf = IntegrateBRDF(uv.x, uv.y);

        output.color = float4(envBrdf, 0, 0);
        return output;
    }

#endif