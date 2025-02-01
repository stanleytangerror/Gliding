
struct VSInput
{
	float2 position : POSITION;
	float3 color : COLOR;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

struct PSOutput
{
	float4 color : COLOR0;
};

PSInput VSMain(VSInput vsin)
{
	PSInput result;

	result.position = float4(vsin.position, 0, 1);
	result.color = float4(vsin.color, 1);

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
}
