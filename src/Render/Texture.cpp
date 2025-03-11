#include "Render/RenderPch.h"
#include "Texture.h"

FileTexture::FileTexture(FrameGraph* frameGraph, const char* filePath, const std::span<const b8>& content)
	: mFilePath(filePath)
	, mTextureExtension(Utils::GetTextureExtension(filePath))
{
	auto image = frameGraph->GetInfra()->CreateFromImageMemory(Utils::GetTextureExtension(filePath), content, filePath);

	auto resource = frameGraph->CreatePermanent(image->GetResourceDesc());

	frameGraph->AddInitialResourcePass("InitialFileTexture", resource,
		[image = std::move(image)](GI::IGraphicsInfra* infra, GI::IGraphicMemoryResource* resource)
		{
			infra->InitialMemoryResourceFromImage(resource, *(image.get()));
		});

	mResource = resource;
}

FileTexture::~FileTexture()
{
	DEBUG_PRINT("Dtor FileTexture %s", mFilePath.c_str());
}

