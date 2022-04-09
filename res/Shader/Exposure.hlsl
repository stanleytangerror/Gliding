
Texture2D SceneHdr;
RWTexture2D<float4> SceneExposureTex;

float4 SceneHdrSize;

[numthreads(32, 32, 1)]
void CSMain(int3 threadId : SV_DispatchThreadID)
{
	const int2 coord = threadId * SceneHdrSize.xy;
	SceneExposureTex[coord] = SceneHdr[coord];
}

