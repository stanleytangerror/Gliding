#ifndef PBR_H
#define PBR_H

Texture2D DiffuseTex : register(t0);
Texture2D NormalTex : register(t1);
Texture2D MetalnessTex : register(t2);
Texture2D LightMapTex : register(t3);
Texture2D SpecularTex : register(t4);
Texture2D BaseColorTex : register(t5);
Texture2D DiffuseRoughnessTex : register(t6);

#endif
