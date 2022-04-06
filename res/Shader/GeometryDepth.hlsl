
struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 worldPos : SV_POSITION;
	float2 uv : TEXCOORD;
};

struct PSOutput
{
	// float4 depth : COLOR0;
};

float4x4 viewMat;
float4x4 projMat;
float4x4 worldMat;

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.worldPos = mul(projMat, mul(viewMat, mul(worldMat, float4(vsin.position, 1))));
	result.uv = vsin.uv;

	return result;
}

PSOutput PSMain(PSInput input) : SV_TARGET
{
	PSOutput output;

	float2 uv = input.uv;
	uv.y = 1.0 - uv.y; // gl texture uv

	return output;
}
