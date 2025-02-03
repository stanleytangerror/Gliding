#include "Render/RenderPch.h"
#include "Texture.h"

FileTexture::FileTexture(FrameGraph* frameGraph, const char* filePath, const std::span<const b8>& content)
	: mFilePath(filePath)
	, mContent(content.begin(), content.end())
	, mTextureExtension(Utils::GetTextureExtension(filePath))
	, mImage(frameGraph->GetInfra()->CreateFromImageMemory(Utils::GetTextureExtension(filePath), std::span(mContent), filePath))
{
	auto resource = frameGraph->CreatePermanent(mImage->GetResourceDesc());

	frameGraph->AddInitialResourcePass("InitialFileTexture", resource,
		[this](GI::IGraphicsInfra* infra, GI::IGraphicMemoryResource* resource)
		{
			infra->InitialMemoryResourceFromImage(resource, *mImage.get());
		});

	mResource = resource;
}

