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
	if (mImportedResources.ContainsValue(resource))
	{
		return { mImportedResources.FindValue(resource).first };
	}

	auto result = FrameGraphMutableResource{ mResourceIdCounter++ };
	Assert(!mImportedResources.ContainsKey(result));
	mImportedResources.Insert(result, resource);

	return result;
}

GI::MemoryResourceDesc ResourceRegistry::GetResourceDesc(const FrameGraphResource& resource) const
{
	if (mTransienceResources.find(resource.mId) != mTransienceResources.end())
	{
		return mTransienceResources.find(resource.mId)->second;
	}
	else if (mImportedResources.ContainsKey(resource.mId))
	{
		auto rawResource = mImportedResources.FindKey(resource.mId).second;
		return GI::MemoryResourceDesc()
			.SetDimension(rawResource->GetDimension())
			.SetWidth(rawResource->GetSize().x())
			.SetHeight(rawResource->GetSize().y())
			.SetDepthOrArraySize(rawResource->GetSize().z())
			.SetFormat(rawResource->GetFormat())
			.SetMipLevels(rawResource->GetMipLevelCount());
	}

	Assert(false);
	return {};
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

SrvUsageFuture RenderPassBuilder::Read(const FrameGraphResource& resource, const GI::SrvDesc& desc)
{
	return { resource, desc };
}

RtvUsageFuture RenderPassBuilder::Write(const FrameGraphMutableResource& resource, const GI::RtvDesc& desc)
{
	return { resource, desc };
}

DsvUsageFuture RenderPassBuilder::Write(const FrameGraphMutableResource& resource, const GI::DsvDesc& desc)
{
	return { resource, desc };
}

UavUsageFuture RenderPassBuilder::Write(const FrameGraphMutableResource& resource, const GI::UavDesc& desc)
{
	return { resource, desc };
}

GI::SrvUsage RenderPassResources::Get(const SrvUsageFuture& usage) const
{
	// TODO
	return GI::SrvUsage();
}

GI::RtvUsage RenderPassResources::Get(const RtvUsageFuture& usage) const
{
	// TODO
	return GI::RtvUsage();
}

GI::DsvUsage RenderPassResources::Get(const DsvUsageFuture& usage) const
{
	// TODO
	return GI::DsvUsage();
}

GI::UavUsage RenderPassResources::Get(const UavUsageFuture& usage) const
{
	// TODO
	return GI::DsvUsage();
}


FrameGraph::FrameGraph(GI::IGraphicsInfra* infra)
	: mInfra(infra)
{
	mBlackboard = std::make_unique<Blackboard>();
	mResourceRegistry = std::make_unique<ResourceRegistry>();
	//mRenderPassBuilder = std::make_unique<RenderPassBuilder>();
}

FrameGraphMutableResource FrameGraph::Create(const GI::MemoryResourceDesc& desc)
{
	return mResourceRegistry->CreateTransientResource(desc);
}

FrameGraphMutableResource FrameGraph::Import(GI::IGraphicMemoryResource* resource)
{
	return mResourceRegistry->ImportResource(resource);
}

GI::MemoryResourceDesc FrameGraph::GetResourceDesc(const FrameGraphResource& resource) const
{
	return mResourceRegistry->GetResourceDesc(resource);
}

