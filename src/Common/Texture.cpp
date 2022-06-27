#include "CommonPch.h"
#include "Texture.h"
#include <array>
#include "CommonTypes.h"

GD_COMMON_API TextureFileExt::Enum Utils::GetTextureExtension(const char* path)
{
	std::filesystem::path ext = std::filesystem::path(path).extension();
	static std::array<const char*, TextureFileExt::Count> ExtNames =
	{ "InvalidExtension", ".dds", ".png", ".bmp", ".gif", ".tiff", ".jpeg", ".jpg" };

	for (i32 i = 0; i < ExtNames.size(); ++i)
	{
		if (ext == ExtNames[i])
		{
			return TextureFileExt::Enum(i);
		}
	}

	return TextureFileExt::Invalid;
}
