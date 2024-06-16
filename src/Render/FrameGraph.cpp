#include "RenderPch.h"
#include "FrameGraph.h"

#define DEBUG_FRAME_GRAPH 1

Blackboard::~Blackboard()
{
	Clear();
}

FrameGraphMutableResource ResourceRegistry::CreateTransientResource(const GI::MemoryResourceDesc& desc)
{
	auto result = FrameGraphMutableResource{ mResourceIdCounter++ };
	Assert(mTransienceResourceDescs.find(result.mId) == mTransienceResourceDescs.end());
	mTransienceResourceDescs[result.mId] = desc;
	return result;
}

FrameGraphMutableResource ResourceRegistry::ImportResource(GI::IGraphicMemoryResource* resource)
{
	if (mImportedResources.ContainsValue(resource))
	{
		return { mImportedResources.FindValue(resource).first };
	}

	auto result = FrameGraphMutableResource{ mResourceIdCounter++ };
	Assert(!mImportedResources.ContainsKey(result.mId));
	mImportedResources.Insert(result.mId, resource);

	return result;
}

GI::IGraphicMemoryResource* ResourceRegistry::GetResource(const FrameGraphResource& resource) const
{
	if (mTransienceResources.find(resource.mId) != mTransienceResources.end())
	{
		return mTransienceResources.find(resource.mId)->second.get();
	}
	else if (mImportedResources.ContainsKey(resource.mId))
	{
		return mImportedResources.FindKey(resource.mId).second;
	}

	Assert(false);
	return nullptr;
}

GI::MemoryResourceDesc ResourceRegistry::GetResourceDesc(const FrameGraphResource& resource) const
{
	if (mTransienceResourceDescs.find(resource.mId) != mTransienceResourceDescs.end())
	{
		return mTransienceResourceDescs.find(resource.mId)->second;
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

void ResourceRegistry::OnSubmitPass(GI::IGraphicsInfra* infra)
{
	for (const auto& [id, desc] : mTransienceResourceDescs)
	{
		mTransienceResources.insert({ id, infra->CreateMemoryResource(desc) });
	}
}

void ResourceRegistry::OnEndFrame()
{
	mTransienceResources.clear();
}

RenderPassBuilder::RenderPassBuilder(FrameGraphBuilder* builder, const char* passName)
	: mBuilder(builder)
	, mPassName(passName)
{
}

GI::VbvUsage	RenderPassBuilder::Read(const GI::VbvUsage& usage)
{
	return usage;
}

GI::IbvUsage	RenderPassBuilder::Read(const GI::IbvUsage& usage)
{
	return usage;
}

GI::SamplerDesc	RenderPassBuilder::Read(const GI::SamplerDesc& usage)
{
	return usage;
}

SrvUsageFuture RenderPassBuilder::Read(const FrameGraphResource& resource, const GI::SrvDesc& desc)
{
	mInputResources.push_back(resource.mId);
	return { resource, desc };
}

UavUsageFuture RenderPassBuilder::Read(const FrameGraphResource& resource, const GI::UavDesc& desc)
{
	mInputResources.push_back(resource.mId);
	return { resource, desc };
}

RtvUsageFuture RenderPassBuilder::Write(const FrameGraphMutableResource& resource, const GI::RtvDesc& desc)
{
	mOutputResources.push_back(resource.mId);
	return { resource, desc };
}

DsvUsageFuture RenderPassBuilder::Write(const FrameGraphMutableResource& resource, const GI::DsvDesc& desc)
{
	mOutputResources.push_back(resource.mId);
	return { resource, desc };
}

UavUsageFuture RenderPassBuilder::Write(const FrameGraphMutableResource& resource, const GI::UavDesc& desc)
{
	mOutputResources.push_back(resource.mId);
	return { resource, desc };
}

RenderPassResources::RenderPassResources(ResourceRegistry* resourceRegistry)
	: mResourceRegistry(resourceRegistry)
{}

GI::SrvUsage RenderPassResources::Get(const SrvUsageFuture& usage) const
{
	auto resource = mResourceRegistry->GetResource(usage.resource);
	auto result = GI::SrvUsage(resource);
	std::memcpy(&result, &(usage.desc), sizeof(GI::SrvDesc));
	return result;
}

GI::RtvUsage RenderPassResources::Get(const RtvUsageFuture& usage) const
{
	auto resource = mResourceRegistry->GetResource(usage.resource);
	auto result = GI::RtvUsage(resource);
	std::memcpy(&result, &(usage.desc), sizeof(GI::RtvDesc));
	return result;
}

GI::DsvUsage RenderPassResources::Get(const DsvUsageFuture& usage) const
{
	auto resource = mResourceRegistry->GetResource(usage.resource);
	auto result = GI::DsvUsage(resource);
	std::memcpy(&result, &(usage.desc), sizeof(GI::DsvDesc));
	return result;
}

GI::UavUsage RenderPassResources::Get(const UavUsageFuture& usage) const
{
	auto resource = mResourceRegistry->GetResource(usage.resource);
	auto result = GI::UavUsage(resource);
	std::memcpy(&result, &(usage.desc), sizeof(GI::UavDesc));
	return result;
}

void FrameGraphBuilder::HandlePassBuilder(const RenderPassBuilder& passBuilder)
{
	auto tryAddResourceNode = [this](const FrameGraphResource::Id& resource)
		{
			auto it = mResourceNodes.find(resource);
			if (it != mResourceNodes.end())
			{
				return it->second;
			}

			auto nodeHandle = mResourceGraph.AddNode();
			mResourceNodes.insert({ resource, nodeHandle });
			return nodeHandle;
		};

	PassHandle passHandle = mPasses.size();
	mPasses.push_back({ passBuilder.mPassName, passBuilder.mPassFunction });
	
	for (const auto& input : passBuilder.mInputResources)
	{
		for (const auto& output : passBuilder.mOutputResources)
		{
			auto inputNode = tryAddResourceNode(input);
			auto outputNode = tryAddResourceNode(output);
			auto edge = mResourceGraph.AddEdge(inputNode, outputNode);
			mPassEdges.insert({ edge, passHandle });
		}
	}
}

void FrameGraphBuilder::MarkOutputNode(const FrameGraphResource& resource)
{
	Assert(mResourceNodes.find(resource.mId) != mResourceNodes.end());
	mPresentResources.insert(resource.mId);
}

void FrameGraphBuilder::SubmitPasses()
{
	Assert(!mPresentResources.empty());
	std::vector<DirectedGraph::NodeHandle> outputNodes(mPresentResources.size());
	std::transform(mPresentResources.begin(), mPresentResources.end(),
		outputNodes.begin(),
		[this](FrameGraphResource::Id id) { return mResourceNodes[id]; });
	
	auto edges = DirectedGraph::CullAndSort(mResourceGraph, outputNodes);

	std::set<PassHandle> finished;
	for (auto e : edges)
	{
		auto it = mPassEdges.find(e);
		Assert(it != mPassEdges.end());

		auto passHandle = it->second;
		const auto& pass = mPasses[passHandle];
		pass.mExecute();

		finished.insert(passHandle);
	}
}

FrameGraph::FrameGraph(GI::IGraphicsInfra* infra)
	: mInfra(infra)
{
	mBlackboard = std::make_unique<Blackboard>();
	mResourceRegistry = std::make_unique<ResourceRegistry>();
}

void FrameGraph::StartFrame()
{
	mFrameGraphBuilder = std::make_unique<FrameGraphBuilder>();
}

void FrameGraph::EndFrame()
{
	mResourceRegistry->OnSubmitPass(mInfra);

	mFrameGraphBuilder->SubmitPasses();
	mFrameGraphBuilder = nullptr;

	mResourceRegistry->OnEndFrame();
}

FrameGraphMutableResource FrameGraph::Create(const GI::MemoryResourceDesc& desc)
{
	return mResourceRegistry->CreateTransientResource(desc);
}

FrameGraphMutableResource FrameGraph::Import(GI::IGraphicMemoryResource* resource)
{
	auto result = mResourceRegistry->ImportResource(resource);

#if DEBUG_FRAME_GRAPH
	DEBUG_PRINT("Import: resource name = %s, graph infra resource id = %d, frame graph resource id = %d", 
		resource->GetDebugName(), resource->GetResourceId(), result.mId);
#endif

	return result;
}

void FrameGraph::Present(FrameGraphMutableResource resource)
{
	Assert(mFrameGraphBuilder != nullptr);

	mFrameGraphBuilder->MarkOutputNode(resource);
}

GI::MemoryResourceDesc FrameGraph::GetResourceDesc(const FrameGraphResource& resource) const
{
	return mResourceRegistry->GetResourceDesc(resource);
}

