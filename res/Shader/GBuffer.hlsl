
struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	// float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 position : SV_POSITION;
	// float3 tangent : TANGENT;
	float3 normal : NORMAL;
	// float3 binormal : BINORMAL;
	float2 uv : TEXCOORD;
};

struct PSOutput
{
	float4 diffuse : COLOR0;
	float4 normal : COLOR1;
	float4 specular : COLOR2;
};

Texture2D DiffuseTex : register(t0);
Texture2D NormalTex : register(t1);
Texture2D SpecularTex : register(t2);
SamplerState SamplerLinear : register(s0);

cbuffer Param : register(b0)
{
	float4x4 viewMat;
	float4x4 projMat;
	float4x4 worldMat;
}

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	float3 pos = vsin.position;
    // pos = float3(0, 0, 0);

	result.position = mul(projMat, mul(viewMat, mul(worldMat, float4(pos, 1))));
	result.normal = vsin.normal;
	// result.tangent = vsin.tangent;

	// x: tangent, y: binormal, z: normal
	// result.binormal = normalize(cross(result.normal, result.tangent));

	// result.uv = float2(vsin.uv.x, 1 - vsin.uv.y);

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	const float4 diffuseColor = float4(0.5, 0.5, 0.5, 1.0);
	// const float4 diffuseColor = DiffuseTex.Sample(SamplerLinear, input.uv).xyzw;
	clip(diffuseColor.w - 0.5);

	// const float3x3 tbn = float3x3(input.tangent, input.binormal, input.normal);

	// const float2 normXy = NormalTex.Sample(SamplerLinear, input.uv).xy * 2 - 1;
	// float3 normal = float3(normXy, sqrt(1 - dot(normXy, normXy)));
	// normal = normalize(mul(normal, tbn));
	float3 normal = normalize(mul(input.normal, (float3x3)worldMat));

	// const float4 spec = SpecularTex.Sample(SamplerLinear, input.uv);
    const float4 spec = float4(0.5, 0.5, 0.5, 1.0);
    float3 specColor = spec.xyz;
	const float gloss = 1 - spec.w;

	output.diffuse = float4(diffuseColor.xyz, 1.0);
	output.normal = float4(normal * 0.5 + 0.5, 1.0);
	output.specular = float4(specColor, gloss);

	return output;
}
