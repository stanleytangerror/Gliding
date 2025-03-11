#pragma once

#include "Common/Texture.h"
#include "Common/GraphicsInfrastructure.h"
#include "FrameGraph.h"

class GD_RENDER_API FileTexture
{
public:
	FileTexture(FrameGraph* frameGraph, const char* filePath, const std::span<const b8>& content);
	~FileTexture();

	FrameGraphResource				GetResource() const { return mResource; }

protected:
	std::string const				mFilePath;
	TextureFileExt::Enum const		mTextureExtension;
	FrameGraphResource				mResource;
};