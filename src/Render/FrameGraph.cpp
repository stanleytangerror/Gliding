#include "RenderPch.h"
#include "FrameGraph.h"
#include <ranges>

#define DEBUG_FRAME_GRAPH 0

Blackboard::~Blackboard()
{
	Clear();
}

FrameGraphMutableResource ResourceRegistry::CreatePermanentResource(const GI::MemoryResourceDesc& desc)
{
	auto resourceId = FrameGraphResource::Id{ mResourceIdCounter++ };
	Assert(mPermanentResources.find(resourceId.mHandle) == mPermanentResources.end());
	mPermanentResources[resourceId.mHandle] = { desc, nullptr };

#if DEBUG_FRAME_GRAPH
	DEBUG_PRINT("[Create] Permanent %d:\t%s", resourceId.GetDebugName().c_str(), desc.GetName());
#endif

	return FrameGraphMutableResource{ resourceId };
}

FrameGraphMutableResource ResourceRegistry::CreateTransientResource(const GI::MemoryResourceDesc& desc)
{
	auto resourceId = FrameGraphResource::Id{ mResourceIdCounter++ };
	Assert(mTransienceResources.find(resourceId.mHandle) == mTransienceResources.end());
	mTransienceResources[resourceId.mHandle] = { desc, nullptr };

#if DEBUG_FRAME_GRAPH
	DEBUG_PRINT("[Create] Transient %d:\t%s", resourceId.GetDebugName().c_str(), desc.GetName());
#endif

	return FrameGraphMutableResource{ resourceId };
}

FrameGraphMutableResource ResourceRegistry::ImportResource(GI::IGraphicMemoryResource* resource)
{
	if (mImportedResources.ContainsValue(resource))
	{
		FrameGraphResource::Id resourceId = { mImportedResources.GetKeyByValue(resource).index };
		return { resourceId };
	}

	auto resourceId = FrameGraphResource::Id{ mResourceIdCounter++ };
	Assert(!mImportedResources.ContainsKey(resourceId.mHandle));
	mImportedResources.Insert(resourceId.mHandle, resource);

	Assert(mImportedResourceDescs.find(resourceId.mHandle) == mImportedResourceDescs.end());
	mImportedResourceDescs[resourceId.mHandle] = GI::MemoryResourceDesc()
		.SetDimension(resource->GetDimension())
		.SetWidth(resource->GetSize().x())
		.SetHeight(resource->GetSize().y())
		.SetDepthOrArraySize(resource->GetSize().z())
		.SetFormat(resource->GetFormat())
		.SetMipLevels(resource->GetMipLevelCount())
		.SetName(resource->GetDebugName());

#if DEBUG_FRAME_GRAPH
	DEBUG_PRINT("[Import] %d:\t%s (reource id %d)",
		resourceId.GetDebugName().c_str(), resource->GetDebugName(), resource->GetResourceId());
#endif

	return FrameGraphMutableResource{ resourceId };
}


void ResourceRegistry::UnimportResource(GI::IGraphicMemoryResource* resource)
{
	if (!mImportedResources.ContainsValue(resource)) { return; }
	
	auto idHandle = mImportedResources.GetKeyByValue(resource);
	mImportedResources.EraseByValue(resource);

	Assert(mImportedResourceDescs.find(idHandle) != mImportedResourceDescs.end());
	mImportedResourceDescs.erase(idHandle);

#if DEBUG_FRAME_GRAPH
	DEBUG_PRINT("[Unimport] %d:\t%s (reource id %d)",
		resourceId.GetDebugName().c_str(), resource->GetDebugName(), resource->GetResourceId());
#endif
}

GI::IGraphicMemoryResource* ResourceRegistry::GetResource(const FrameGraphResource& resource) const
{
	if (mTransienceResources.find(resource.mId.mHandle) != mTransienceResources.end())
	{
		return mTransienceResources.find(resource.mId.mHandle)->second.mRealResource.get();
	}
	else if (mPermanentResources.find(resource.mId.mHandle) != mPermanentResources.end())
	{
		return mPermanentResources.find(resource.mId.mHandle)->second.mRealResource.get();
	}
	else if (mImportedResources.ContainsKey(resource.mId.mHandle))
	{
		return mImportedResources.GetValueByKey(resource.mId.mHandle);
	}

	Assert(false);
	return nullptr;
}

GI::MemoryResourceDesc ResourceRegistry::GetResourceDesc(const FrameGraphResource& resource) const
{
	if (mTransienceResources.find(resource.mId.mHandle) != mTransienceResources.end())
	{
		return mTransienceResources.find(resource.mId.mHandle)->second.mDesc;
	}
	else if (mPermanentResources.find(resource.mId.mHandle) != mPermanentResources.end())
	{
		return mPermanentResources.find(resource.mId.mHandle)->second.mDesc;
	}
	else if (mImportedResourceDescs.find(resource.mId.mHandle) != mImportedResourceDescs.end())
	{
		return mImportedResourceDescs.find(resource.mId.mHandle)->second;
	}

	Assert(false);
	return {};
}

void ResourceRegistry::OnCompile(GI::IGraphicsInfra* infra)
{
	for (auto& [id, data] : mPermanentResources)
	{
		if (!data.mRealResource)
		{
			data.mRealResource = infra->CreateMemoryResource(data.mDesc);
		}
	}
	for (auto& [id, data] : mTransienceResources)
	{
		Assert(data.mRealResource == nullptr);

		auto descKey = data.mDesc.GetKey();
		if (mTransicenceResourcePool.find(descKey) != mTransicenceResourcePool.end())
		{
			auto& items = mTransicenceResourcePool.find(descKey)->second;
			std::swap(data.mRealResource, items.back().mRealResource);
			items.pop_back();
			// TODO reset debug name to data.mRealResource
		}
		else
		{
			data.mRealResource = infra->CreateMemoryResource(data.mDesc);
		}
	}
}

void ResourceRegistry::OnEndFrame()
{
	for (auto it = mTransienceResources.begin(); it != mTransienceResources.end();)
	{
		auto& resourceData = it->second;

		const auto key = resourceData.mDesc.GetKey();
		if (mTransicenceResourcePool.find(key) == mTransicenceResourcePool.end())
		{
			mTransicenceResourcePool.insert({ key, std::vector<ResourcePoolItem>{} });
		}
		auto& poolItems = mTransicenceResourcePool.find(key)->second;
		poolItems.push_back({});
		auto& poolItem = poolItems.back();

		poolItem.mDesc = resourceData.mDesc;
		std::swap(poolItem.mRealResource, resourceData.mRealResource);
		poolItem.mIdleFrames += 1;

		it = mTransienceResources.erase(it);
	}

	for (auto& [_, items] : mTransicenceResourcePool)
	{
		for (auto it = items.begin(); it != items.end();)
		{
			it = it->IdleTooLong() ? it = items.erase(it) : ++it;
		}
	}
}

RenderPassBuilder::RenderPassBuilder(FrameGraphBuilder* builder, const char* passName)
	: mBuilder(builder)
	, mPassName(passName)
{
}

VbvUsageFuture	RenderPassBuilder::ReadVbv(const FrameGraphResource& resource, const GI::VbvDesc& desc)
{
	Assert(resource.IsValid());
	mInputResources.push_back(resource.mId);
	return { resource, desc };
}

IbvUsageFuture	RenderPassBuilder::ReadIbv(const FrameGraphResource& resource, const GI::IbvDesc& desc)
{
	Assert(resource.IsValid());
	mInputResources.push_back(resource.mId);
	return { resource, desc };
}

GI::SamplerDesc	RenderPassBuilder::Read(const GI::SamplerDesc& usage)
{
	return usage;
}

FrameGraphResource RenderPassBuilder::Read(const FrameGraphResource& resource)
{
	Assert(resource.IsValid());

	mInputResources.push_back(resource.mId);
	return resource;
}


SrvUsageFuture RenderPassBuilder::ReadTex2DSrv(const FrameGraphResource& resource)
{
	Assert(resource.IsValid());

	const auto& desc = mBuilder->GetResourceRegistry()->GetResourceDesc(resource);
	return Read(resource, GI::MemoryResourceDesc::AsTexture2DSrv(desc));
}

SrvUsageFuture RenderPassBuilder::ReadBufferSrv(const FrameGraphResource& resource, u32 numElements, u32 stride)
{
	Assert(resource.IsValid());

	const auto& desc = mBuilder->GetResourceRegistry()->GetResourceDesc(resource);
	return Read(resource, GI::MemoryResourceDesc::AsBufferSrv(desc, numElements, stride));
}

SrvUsageFuture RenderPassBuilder::Read(const FrameGraphResource& resource, const GI::SrvDesc& desc)
{
	Assert(resource.IsValid());

	mInputResources.push_back(resource.mId);
	return { resource, desc };
}

UavUsageFuture RenderPassBuilder::Read(const FrameGraphResource& resource, const GI::UavDesc& desc)
{
	Assert(resource.IsValid());

	mInputResources.push_back(resource.mId);
	return { resource, desc };
}

RtvUsageFuture RenderPassBuilder::WriteTex2DRtv(FrameGraphMutableResource& resource)
{
	Assert(resource.IsValid());

	const auto& desc = mBuilder->GetResourceRegistry()->GetResourceDesc(resource);
	return Write(resource, GI::MemoryResourceDesc::AsTexture2DRtv(desc));
}

RtvUsageFuture RenderPassBuilder::Write(FrameGraphMutableResource& resource, const GI::RtvDesc& desc)
{
	Assert(resource.IsValid());

	resource.IncrementVersion();
	mOutputResources.push_back(resource.mId);
	return { resource, desc };
}

DsvUsageFuture RenderPassBuilder::WriteTex2DDsv(FrameGraphMutableResource& resource)
{
	Assert(resource.IsValid());

	const auto& desc = mBuilder->GetResourceRegistry()->GetResourceDesc(resource);
	return Write(resource, GI::MemoryResourceDesc::AsTexture2DDsv(desc));
}

DsvUsageFuture RenderPassBuilder::Write(FrameGraphMutableResource& resource, const GI::DsvDesc& desc)
{
	Assert(resource.IsValid());

	resource.IncrementVersion();
	mOutputResources.push_back(resource.mId);
	return { resource, desc };
}

UavUsageFuture RenderPassBuilder::WriteBufferUav(FrameGraphMutableResource& resource, u32 numElements, u32 stride)
{
	Assert(resource.IsValid());

	const auto& desc = mBuilder->GetResourceRegistry()->GetResourceDesc(resource);
	return Write(resource, GI::MemoryResourceDesc::AsBufferUav(desc, numElements, stride));
}

UavUsageFuture RenderPassBuilder::Write(FrameGraphMutableResource& resource, const GI::UavDesc& desc)
{
	Assert(resource.IsValid());

	resource.IncrementVersion();
	mOutputResources.push_back(resource.mId);
	return { resource, desc };
}

FrameGraphMutableResource RenderPassBuilder::Write(FrameGraphMutableResource& resource)
{
	Assert(resource.IsValid());

	resource.IncrementVersion();
	mOutputResources.push_back(resource.mId);
	return resource;
}

DsvUsageFuture RenderPassBuilder::ReadWriteTex2DDsv(FrameGraphMutableResource& resource)
{
	Assert(resource.IsValid());

	const auto& desc = mBuilder->GetResourceRegistry()->GetResourceDesc(resource);
	return ReadWrite(resource, GI::MemoryResourceDesc::AsTexture2DDsv(desc));
}

DsvUsageFuture RenderPassBuilder::ReadWrite(FrameGraphMutableResource& resource, const GI::DsvDesc& desc)
{
	Assert(resource.IsValid());

	mInputResources.push_back(resource.mId);

	resource.IncrementVersion();
	mOutputResources.push_back(resource.mId);
	return { resource, desc };
}


FrameGraphMutableResource RenderPassBuilder::ReadWrite(FrameGraphMutableResource& resource)
{
	Assert(resource.IsValid());

	mInputResources.push_back(resource.mId);

	resource.IncrementVersion();
	mOutputResources.push_back(resource.mId);
	return resource;
}


void RenderPassBuilder::MarkSideEffect(const FrameGraphResource& resource)
{
	Assert(resource.IsValid());

	mSideEffectResources.push_back(resource.mId);
}

UavUsageFuture RenderPassBuilder::ReadWriteTex2DUav(FrameGraphMutableResource& resource)
{
	Assert(resource.IsValid());

	const auto& desc = mBuilder->GetResourceRegistry()->GetResourceDesc(resource);
	return ReadWrite(resource, GI::MemoryResourceDesc::AsTexture2DUav(desc));
}


UavUsageFuture RenderPassBuilder::ReadWrite(FrameGraphMutableResource& resource, const GI::UavDesc& desc)
{
	Assert(resource.IsValid());

	mInputResources.push_back(resource.mId);

	resource.IncrementVersion();
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

GI::VbvUsage RenderPassResources::Get(const VbvUsageFuture& usage) const
{
	auto resource = mResourceRegistry->GetResource(usage.resource);
	auto result = GI::VbvUsage(resource);
	std::memcpy(&result, &(usage.desc), sizeof(GI::VbvDesc));
	return result;
}

GI::IbvUsage RenderPassResources::Get(const IbvUsageFuture& usage) const
{
	auto resource = mResourceRegistry->GetResource(usage.resource);
	auto result = GI::IbvUsage(resource);
	std::memcpy(&result, &(usage.desc), sizeof(GI::IbvDesc));
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

			return mResourceNodes.GetValueByKey(resourceId);
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

		// TODO reduce versions
		if (output.mVersion.index > 0)
		{
			auto prevVersion = FrameGraphResource::Id(output);
			prevVersion.mVersion.index = output.mVersion.index - 1;
			if (mResourceNodes.ContainsKey(prevVersion))
			{
				mResourceGraph.AddEdge(mResourceNodes.GetValueByKey(prevVersion), outputNode);
			}
		}
	}

	for (auto resourceId : passBuilder.mSideEffectResources)
	{
		MarkOutputNode(resourceId);
	}
}

void FrameGraphBuilder::MarkOutputNode(const FrameGraphResource::Id& resourceId)
{
	Assert(mResourceNodes.ContainsKey(resourceId));
	mOutputResources.insert(resourceId);
}

void FrameGraphBuilder::CompileAndExecute()
{
	PROFILE_EVENT(FrameGraphBuilder::CompileAndExecute);

	Assert(!mOutputResources.empty());

	DirectedGraph::Cull(mResourceGraph, mOutputResources
		| std::views::transform([this](const FrameGraphResource::Id& id) { return mResourceNodes.GetValueByKey(id); }));

#if DEBUG_FRAME_GRAPH
	auto serialized = DirectedGraph::Serialize(mResourceGraph, [this](auto n)
		{
			if (mResourceNodes.ContainsValue(n))
			{
				const auto& id = mResourceNodes.GetKeyByValue(n);
				const auto& name = mResourceRegistry->GetResourceDesc({ id }).GetName();
				return std::make_tuple(
					Utils::FormatString("%s\\nId: %s", Utils::EscapeString(name.c_str()).c_str(), id.GetDebugName().c_str()),
					"resource");
			}
			else
			{
				auto passHandle = mPassNodes.GetKeyByValue(n);
				return std::make_tuple(
					Utils::FormatString("%s\\nPassHandle: %d", mPasses[passHandle].mPassName.c_str(), passHandle),
					"pass");
			}
		});

	Utils::WriteFileText(R"(res/Tool/graph.json)", serialized);
#endif

	const auto& nodes = DirectedGraph::TopoSort(mResourceGraph);

	std::vector<Pass> sortedPasses;
	for (auto n : nodes)
	{
		if (mPassNodes.ContainsValue(n))
		{
			sortedPasses.push_back(mPasses[mPassNodes.GetKeyByValue(n)]);

#if DEBUG_FRAME_GRAPH
			DebugOutputPassNode(n, "[Pass] ");
#endif
		}
	}

	{
		PROFILE_EVENT(Execute);
		for (const auto& pass : sortedPasses)
		{
			pass.mExecute();
		}
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

		auto inputs = mResourceGraph.GetIncomingNodesRef(node);
		for (const auto& n : inputs) { DebugOutputResourceNode(n, "\t - "); }

		auto outputs = mResourceGraph.GetOutgoingNodesRef(node);
		for (const auto& n : outputs) { DebugOutputResourceNode(n, "\t + "); }
	}
}

void FrameGraphBuilder::DebugOutputResourceNode(DirectedGraph::NodeHandle node, const char* prefix)
{
	Assert(mResourceNodes.ContainsValue(node));

	const auto& id = mResourceNodes.GetKeyByValue(node);
	const auto& name = mResourceRegistry->GetResourceDesc({ id }).GetName();
	DEBUG_PRINT("%s[node:%d]: %d\t%s", prefix ? prefix : "", node, id, name.c_str());
}

void FrameGraphBuilder::DebugOutputPassNode(DirectedGraph::NodeHandle node, const char* prefix)
{
	Assert(mPassNodes.ContainsValue(node));
	
	auto passHandle = mPassNodes.GetKeyByValue(node);
	DEBUG_PRINT("%s[node:%d]: %d\t%s", prefix ? prefix : "", node, passHandle, mPasses[passHandle].mPassName.c_str());
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
	mResourceRegistry->OnCompile(mInfra);

	mFrameGraphBuilder->CompileAndExecute();
	mFrameGraphBuilder = nullptr;

	mResourceRegistry->OnEndFrame();
}

FrameGraphMutableResource FrameGraph::CreatePermanent(const GI::MemoryResourceDesc& desc)
{
	return mResourceRegistry->CreatePermanentResource(desc);
}

FrameGraphMutableResource FrameGraph::CreateTransient(const GI::MemoryResourceDesc& desc)
{
	return mResourceRegistry->CreateTransientResource(desc);
}

FrameGraphMutableResource FrameGraph::Import(GI::IGraphicMemoryResource* resource)
{
	return mResourceRegistry->ImportResource(resource);
}


void FrameGraph::Unimport(GI::IGraphicMemoryResource* resource)
{
	mResourceRegistry->UnimportResource(resource);
}

void FrameGraph::Present(FrameGraphMutableResource resource)
{
	Assert(mFrameGraphBuilder != nullptr);

	mFrameGraphBuilder->MarkOutputNode(resource.mId);
}

GI::MemoryResourceDesc FrameGraph::GetResourceDesc(const FrameGraphResource& resource) const
{
	return mResourceRegistry->GetResourceDesc(resource);
}


void FrameGraph::AddClearPass(const char* name, std::vector<FrameGraphMutableResource> renderTargets, const Vec4f& colorValue, FrameGraphMutableResource depthStencil, bool clearDepth, f32 depthValue, bool clearStencil, u32 stencilValue)
{
	struct PassData
	{
		std::vector<RtvUsageFuture> rtvs;
		bool clearDsv = false;
		DsvUsageFuture dsv;
	};

	AddPass<PassData>(name,
		[&](RenderPassBuilder& builder, PassData& data)
		{
			for (auto rt : renderTargets)
			{
				data.rtvs.push_back(builder.WriteTex2DRtv(rt));
			}
			data.clearDsv = depthStencil.IsValid() && (clearDepth || clearStencil);
			if (data.clearDsv)
			{
				data.dsv = builder.WriteTex2DDsv(depthStencil);
			}
		},
		[=](const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			for (const auto& rt : data.rtvs)
			{
				infra->GetRecorder()->AddClearOperation(resources.Get(rt), colorValue);
			}
			if (data.clearDsv)
			{
				infra->GetRecorder()->AddClearOperation(resources.Get(data.dsv), clearDepth, depthValue, clearStencil, stencilValue);
			}
		});
}

void FrameGraph::AddInitialResourcePass(const char* name, FrameGraphMutableResource& resource, std::function<void(GI::IGraphicsInfra*, GI::IGraphicMemoryResource*)> write)
{
	struct PassData
	{
		FrameGraphMutableResource targetResource;
	};

	AddPass<PassData>(name,
		[&](RenderPassBuilder& builder, PassData& data)
		{
			data.targetResource = builder.Write(resource);
			builder.MarkSideEffect(data.targetResource);
		},
		[write](const PassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)
		{
			write(infra, resources.Get(data.targetResource.mId));
		});
}