#pragma once

enum TextureType
{
	TextureType_1d = 5,
	TextureType_1dArray = 4,
	TextureType_2d = 0,
	TextureType_2dArray = 3,
	TextureType_Cube = 1,
	TextureType_3d = 2,
	TextureType_CubeArray = 6,
	TextureTypeCount = 7,
};

enum RenderFormat
{
	RenderFormat_Unknown = 0,
	RenderFormat_R8G8B8A8_UNORM,
};