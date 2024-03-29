#ifndef COMMON_H
#define COMMON_H

#define PI (3.1415927)
#define InvPI (1.0 / PI)

#define EPS	(1e-5)

float Pow4(float x)
{
	float xx = x * x;
	return xx * xx;
}

float Pow5(float x)
{
	float xx = x * x;
	return xx * xx * x;
}

float4 UnpackColorABGRU32(uint v)
{
	float4 channels = float4(
		float(0x000000ff & (v >> 24)),
		float(0x000000ff & (v >> 16)),
		float(0x000000ff & (v >> 8)	),
		float(0x000000ff & (v >> 0)	)
	);
	return channels.wzyx / 255.0;
}

float LinearToSrgbChannel(float linearColor)
{
	return (linearColor < 0.00313067) ?
            linearColor * 12.92 :
	        pow(linearColor, (1.0/2.4)) * 1.055 - 0.055;
}

float3 LinearToSrgb(float3 linearColor)
{
	return float3(
        LinearToSrgbChannel(linearColor.x),
        LinearToSrgbChannel(linearColor.y),
        LinearToSrgbChannel(linearColor.z)
    );
}

float3 sRGBToLinear(float3 srgbColor)
{
	srgbColor = max(6.10352e-5, srgbColor);
	return srgbColor > 0.04045 ? 
            pow( srgbColor * (1.0 / 1.055) + 0.0521327, 2.4 ) : 
            srgbColor * (1.0 / 12.92);
}

float GetLuminance(float3 color)
{
	return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

#endif
