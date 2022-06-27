#pragma once

struct TextureFileExt
{
	enum Enum
	{
		Invalid, DDS, PNG, BMP, GIF, TIFF, JPEG, JPG, Count
	};
};

namespace Utils
{
	GD_COMMON_API TextureFileExt::Enum GetTextureExtension(const char* path);
}
