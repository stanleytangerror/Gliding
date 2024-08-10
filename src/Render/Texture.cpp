#include "RenderPch.h"
#include "Texture.h"

FileTexture::FileTexture(FrameGraph* frameGraph, const char* filePath, const std::vector<b8>& content)
	: mFilePath(filePath)
	, mContent(content)
	, mTextureExtension(Utils::GetTextureExtension(filePath))
	, mImage(frameGraph->GetInfra()->CreateFromImageMemory(Utils::GetTextureExtension(filePath), mContent, filePath))
{
	mResource = frameGraph->CreatePermanent(
		mImage->GetResourceDesc(),
		[this](GI::IGraphicsInfra* infra, GI::IGraphicMemoryResource* resource)
		{
			infra->InitialMemoryResourceFromImage(resource, *mImage.get());
		});
}

