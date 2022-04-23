#include "Common.h"
#include "Time.h"

#if CONSTRUCT_HISTOGRAM

Texture2D SceneHdr;
float4 SceneHdrSize;

RWStructuredBuffer<uint> SceneBrightnessHistogram;
float4 HistogramInfo;
#define HistXMin	(HistogramInfo.x)
#define HistXMax	(HistogramInfo.y)
#define HistXRes	(HistogramInfo.z)
#define HistInvXRes	(HistogramInfo.w)

#define HISTOGRAM_MAX_RESOLUTION 256
groupshared uint HistogramInGroup[HISTOGRAM_MAX_RESOLUTION];

[numthreads(32, 32, 1)]
void CSMain(uint2 threadIdGlobal : SV_DispatchThreadID, uint2 threadIdInGroup : SV_GroupThreadID)
{
	const uint localThreadId = threadIdInGroup.y * 32 + threadIdInGroup.x;

	if (localThreadId < HISTOGRAM_MAX_RESOLUTION)
	{
		HistogramInGroup[localThreadId] = 0;
	}

    GroupMemoryBarrierWithGroupSync();

	if (threadIdGlobal.x < SceneHdrSize.x && threadIdGlobal.y < SceneHdrSize.y)
	{
		const float3 sceneColor = SceneHdr[threadIdGlobal].xyz;
		const float logLum = log2(GetLuminance(sceneColor));
		const uint histX = saturate((logLum - HistXMin) / (HistXMax - HistXMin)) * HistXRes;
        InterlockedAdd(HistogramInGroup[histX], 1);
	}

    GroupMemoryBarrierWithGroupSync();

    if (localThreadId < HISTOGRAM_MAX_RESOLUTION)
    {
		InterlockedAdd(SceneBrightnessHistogram[localThreadId], HistogramInGroup[localThreadId]);
	}

}

#elif HISTOGRAM_REDUCE

RWTexture2D<float4> ExposureTexture;

StructuredBuffer<uint> SceneBrightnessHistogram;
float4 HistogramInfo;
#define HistXMin	(HistogramInfo.x)
#define HistXMax	(HistogramInfo.y)
#define HistXRes	(HistogramInfo.z)
#define HistInvXRes	(HistogramInfo.w)

float4 SceneHdrSize;

float4 EyeAdaptInfo;
#define EYE_ADAPT_SPEED_UP		(EyeAdaptInfo.x)
#define EYE_ADAPT_SPEED_DOWN	(EyeAdaptInfo.y)

[numthreads(1, 1, 1)]
void CSMain(uint2 threadIdGlobal : SV_DispatchThreadID, uint2 threadIdInGroup : SV_GroupThreadID)
{
	float avgLum = 0;
	float invTotalWeight = SceneHdrSize.z * SceneHdrSize.w;
	for (uint i = 0; i < HistXRes; ++i)
	{
		uint y = SceneBrightnessHistogram[i];
		float x = i * HistInvXRes * (HistXMax - HistXMin) + HistXMin;
		float lum = pow(2, x);
		avgLum += lum * y * invTotalWeight;
	}
	avgLum = clamp(avgLum, pow(2, HistXMin), pow(2, HistXMax));

	float2 lastExposure = ExposureTexture[uint2(0, 0)].zw;
	float lastLum = clamp(lastExposure.y, pow(2, HistXMin), pow(2, HistXMax));

	float lastLogLum = log2(lastLum);
	float targetLogLum = log2(avgLum);

	if (targetLogLum < lastLogLum)
	{
		targetLogLum = max(lastLogLum - EYE_ADAPT_SPEED_DOWN * GetDeltaTime(), targetLogLum);
	}
	else
	{
		targetLogLum = min(lastLogLum + EYE_ADAPT_SPEED_UP * GetDeltaTime(), targetLogLum);
	}

	float invExposure = pow(2, targetLogLum);
	float exposure = 1.f / invExposure;
	ExposureTexture[uint2(0, 0)] = float4(exposure, invExposure, lastExposure);
}

#endif