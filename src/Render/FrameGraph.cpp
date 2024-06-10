#include "RenderPch.h"
#include "FrameGraph.h"

Blackboard::~Blackboard()
{
	Clear();
}

FrameGraphMutableResource ResourceRegistry::CreateTransientResource(const GI::MemoryResourceDesc& desc)
{
	auto result = FrameGraphMutableResource{ mResourceIdCounter++ };
	Assert(mTransienceResources.find(result) == mTransienceResources.end());
	mTransienceResources[result] = desc;
	return result;
}

FrameGraphMutableResource ResourceRegistry::ImportResource(GI::IGraphicMemoryResource* resource)
{
	auto result = FrameGraphMutableResource{ mResourceIdCounter++ };
	Assert(mImportedResources.find(result) == mImportedResources.end());
	mImportedResources[result] = resource;
	return result;
}

#define DEBUG_RENDER_PASS_BUILDER 1
RenderPassBuilder::RenderPassBuilder(const char* passName)
	: mPassName(passName)
{
#if DEBUG_RENDER_PASS_BUILDER
	Utils::FormatString("[Pass] %s\n", mPassName.c_str());
#endif
}

GI::VbvUsage	RenderPassBuilder::Read(const GI::VbvUsage& usage)
{
	return usage;
}

GI::IbvUsage	RenderPassBuilder::Read(const GI::IbvUsage& usage)
{
	return usage;
}

GI::SrvUsage	RenderPassBuilder::Read(const GI::SrvUsage& usage)
{
	return usage;
}

GI::SrvUsage	RenderPassBuilder::Read(GI::IGraphicMemoryResource* resource, const GI::SrvDesc& desc)
{
	auto result = GI::SrvUsage(resource);
	std::memcpy(&result, &desc, sizeof(GI::SrvDesc));
	return result;
}

GI::SamplerDesc	RenderPassBuilder::Read(const GI::SamplerDesc& usage)
{
	return usage;

}

GI::UavUsage	RenderPassBuilder::Write(const GI::UavUsage& usage)
{
	return usage;

}

GI::RtvUsage	RenderPassBuilder::Write(const GI::RtvUsage& usage)
{
	return usage;
}

GI::RtvUsage	RenderPassBuilder::Write(GI::IGraphicMemoryResource* resource, const GI::RtvDesc& desc)
{
	auto result = GI::RtvUsage(resource);
	std::memcpy(&result, &desc, sizeof(GI::RtvDesc));
	return result;
}

GI::DsvUsage	RenderPassBuilder::Write(const GI::DsvUsage& usage)
{
	return usage;
}

FrameGraph::FrameGraph(GI::IGraphicsInfra* infra)
	: mInfra(infra)
{
	mBlackboard = std::make_unique<Blackboard>();
	mResourceRegistry = std::make_unique<ResourceRegistry>();
	//mRenderPassBuilder = std::make_unique<RenderPassBuilder>();
}

