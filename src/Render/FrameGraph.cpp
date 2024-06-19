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

#if DEBUG_FRAME_GRAPH
	DEBUG_PRINT("[Create] %d:\t%s",
		result.mId, desc.GetName());
#endif

	return result;
}

FrameGraphMutableResource ResourceRegistry::ImportResource(GI::IGraphicMemoryResource* resource)
{
	if (mImportedResources.ContainsValue(resource))
	{
		return { mImportedResources.GetByValue(resource).first };
	}

	auto result = FrameGraphMutableResource{ mResourceIdCounter++ };
	Assert(!mImportedResources.ContainsKey(result.mId));
	mImportedResources.Insert(result.mId, resource);

#if DEBUG_FRAME_GRAPH
	DEBUG_PRINT("[Import] %d:\t%s (reource id %d)",
		result.mId, resource->GetDebugName(), resource->GetResourceId());
#endif

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
		return mImportedResources.GetByKey(resource.mId).second;
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
		auto rawResource = mImportedResources.GetByKey(resource.mId).second;
		return GI::MemoryResourceDesc()
			.SetDimension(rawResource->GetDimension())
			.SetWidth(rawResource->GetSize().x())
			.SetHeight(rawResource->GetSize().y())
			.SetDepthOrArraySize(rawResource->GetSize().z())
			.SetFormat(rawResource->GetFormat())
			.SetMipLevels(rawResource->GetMipLevelCount())
			.SetName(rawResource->GetDebugName());
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

FrameGraphResource RenderPassBuilder::Read(const FrameGraphResource& resource)
{
	mInputResources.push_back(resource.mId);
	return resource;
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

FrameGraphMutableResource RenderPassBuilder::Write(const FrameGraphMutableResource& resource)
{
	mOutputResources.push_back(resource.mId);
	return resource;
}

UavUsageFuture RenderPassBuilder::ReadWrite(const FrameGraphResource& resource, const GI::UavDesc& desc)
{
	mInputResources.push_back(resource.mId);
	mOutputResources.push_back(resource.mId);
	return { resource, desc };
}

DsvUsageFuture RenderPassBuilder::ReadWrite(const FrameGraphResource& resource, const GI::DsvDesc& desc)
{
	mInputResources.push_back(resource.mId);
	mOutputResources.push_back(resource.mId);
	return { resource, desc };
}

RenderPassResources::RenderPassResources(ResourceRegistry* resourceRegistry)
	: mResourceRegistry(resourceRegistry)
{}

GI::IGraphicMemoryResource* RenderPassResources::Get(const FrameGraphResource::Id& resource) const
{
	return mResourceRegistry->GetResource({ resource });
}

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

FrameGraphBuilder::FrameGraphBuilder(ResourceRegistry* registry)
	: mResourceRegistry(registry)
{
}

void FrameGraphBuilder::HandlePassBuilder(const RenderPassBuilder& passBuilder)
{
	PassHandle passHandle = mPasses.size();
	mPasses.push_back({ passBuilder.mPassName, passBuilder.mPassFunction });
	const auto passNode = mResourceGraph.AddNode();

	mPassNodes.Insert(passHandle, passNode);

	auto tryAddResourceNode = [this](const FrameGraphResource::Id& resourceId)
		{
			if (!mResourceNodes.ContainsKey(resourceId))
			{
				mResourceNodes.Insert(resourceId, mResourceGraph.AddNode());
			}

			return mResourceNodes.GetByKey(resourceId).second;
		};

	for (const auto& input : passBuilder.mInputResources)
	{
		auto inputNode = tryAddResourceNode(input);
		mResourceGraph.AddEdge(inputNode, passNode);
	}

	for (const auto& output : passBuilder.mOutputResources)
	{
		auto outputNode = tryAddResourceNode(output);
		mResourceGraph.AddEdge(passNode, outputNode);
	}
}

void FrameGraphBuilder::MarkOutputNode(const FrameGraphResource& resource)
{
	Assert(mResourceNodes.ContainsKey(resource.mId));
	mPresentResources.insert(resource.mId);
}

void FrameGraphBuilder::CompileAndExecute()
{
	Assert(!mPresentResources.empty());
	std::vector<DirectedGraph::NodeHandle> outputNodes(mPresentResources.size());
	std::transform(mPresentResources.begin(), mPresentResources.end(),
		outputNodes.begin(),
		[this](FrameGraphResource::Id id) { return mResourceNodes.GetByKey(id).second; });
	
#if DEBUG_FRAME_GRAPH
	DebugOutputGraph();
#endif

	auto nodes = DirectedGraph::CullAndSort(mResourceGraph, outputNodes);

#if DEBUG_FRAME_GRAPH
	DEBUG_PRINT("Culled");
#endif

	std::vector<Pass> sortedPasses;
	for (auto n : nodes)
	{
		if (mPassNodes.ContainsValue(n))
		{
			sortedPasses.push_back(mPasses[mPassNodes.GetByValue(n).first]);

#if DEBUG_FRAME_GRAPH
			DebugOutputPassNode(n, "[Pass] ");
#endif
		}
	}

	for (const auto& pass : sortedPasses)
	{
		pass.mExecute();
	}
}

void FrameGraphBuilder::DebugOutputGraph()
{
	for (const auto& [resourceId, node] : mResourceNodes)
	{
		DebugOutputResourceNode(node, "[Resource] ");
	}

	for (const auto& [passHandle, node] : mPassNodes)
	{
		DebugOutputPassNode(node, "[Pass] ");

		auto inputs = mResourceGraph.GetIncomingNodes(node);
		for (const auto& n : inputs) { DebugOutputResourceNode(n, "\t - "); }

		auto outputs = mResourceGraph.GetOutgoingNodes(node);
		for (const auto& n : outputs) { DebugOutputResourceNode(n, "\t + "); }
	}
}

void FrameGraphBuilder::DebugOutputResourceNode(DirectedGraph::NodeHandle node, const char* prefix)
{
	Assert(mResourceNodes.ContainsValue(node));

	const auto& id = mResourceNodes.GetByValue(node).first;
	const char* name = mResourceRegistry->GetResourceDesc({ id }).GetName();
	DEBUG_PRINT("%s[node:%d]: %d\t%s", prefix ? prefix : "", node, id, name);
}

void FrameGraphBuilder::DebugOutputPassNode(DirectedGraph::EdgeHandle edge, const char* prefix)
{
	Assert(mPassNodes.ContainsValue(edge));
	
	auto passHandle = mPassNodes.GetByValue(edge).first;
	DEBUG_PRINT("%s[edge:%d]: %d\t%s", prefix ? prefix : "", edge, passHandle, mPasses[passHandle].mPassName.c_str());
}

FrameGraph::FrameGraph(GI::IGraphicsInfra* infra)
	: mInfra(infra)
{
	mBlackboard = std::make_unique<Blackboard>();
	mResourceRegistry = std::make_unique<ResourceRegistry>();
}

void FrameGraph::StartFrame()
{
	mFrameGraphBuilder = std::make_unique<FrameGraphBuilder>(mResourceRegistry.get());
}

void FrameGraph::EndFrame()
{
	mResourceRegistry->OnSubmitPass(mInfra);

	mFrameGraphBuilder->CompileAndExecute();
	mFrameGraphBuilder = nullptr;

	mResourceRegistry->OnEndFrame();
}

FrameGraphMutableResource FrameGraph::Create(const GI::MemoryResourceDesc& desc)
{
	return mResourceRegistry->CreateTransientResource(desc);
}

FrameGraphMutableResource FrameGraph::Import(GI::IGraphicMemoryResource* resource)
{
	return mResourceRegistry->ImportResource(resource);
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

