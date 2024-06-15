#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "Common/CommonUtils.h"
#include "Common/Container.h"
#include "Common/DirectedGraph.h"

class RenderPassResources;

class GD_RENDER_API Blackboard
{
public:
	virtual ~Blackboard();

	template<typename T>
	T& Get() const
	{
		auto* const item = static_cast<Item<T>*>(mItems[GetSlotIndex<T>()]);
		return *item;
	}

	template<class T, typename... Args>
	T& Add(Args&... args)
	{
		const auto slotIndex = InitialSlotIndex<T>();

		Assert(mItems[slotIndex] == nullptr);
		auto * const newItem = new Item<T>(std::forward<Args>(args)...);
		mItems[slotIndex] = newItem;

		return *newItem;
	}

	template<typename T>
	void Remove()
	{
		Utils::SafeDelete(mItems[GetSlotIndex<T>()]);
	}

	void Clear()
	{
		for (auto e : mItems)
		{
			Utils::SafeDelete(e);
		}
	}

	struct ItemBase
	{
	};

	template<typename T>
	struct Item : public ItemBase, T
	{
		T mContent;
		inline static u16 sSlotIndexCache = Blackboard::sInvalidSlotIndex; // used to speed up mTypeToSlotIndex lookup
	};

private:
	template<typename T>
	u16 InitialSlotIndex()
	{
		const auto& name = typeid(T).name();
		Assert(mTypeToSlotIndex.find(name) == mTypeToSlotIndex.end());

		mTypeToSlotIndex[name] = sSlotCounter;
		Item<T>::sSlotIndexCache = sSlotCounter;
		sSlotCounter++;

		while (mItems.size() < sSlotCounter)
		{
			mItems.push_back(nullptr);
		}

		return Item<T>::sSlotIndexCache;
	}

	template<typename T>
	u16 GetSlotIndex() const
	{
		if (Item<T>::sSlotIndexCache == sInvalidSlotIndex)
		{
			const auto& name = typeid(T).name();

			auto it = mTypeToSlotIndex.find(name);
			Assert(it != mTypeToSlotIndex.end());
			Item<T>::sSlotIndexCache = it->second;
		}

		return Item<T>::sSlotIndexCache;
	}

private:
	std::vector<ItemBase*>		mItems;
	std::map<std::string, u16>	mTypeToSlotIndex;
	u16							sSlotCounter = 0;
	inline static const u16		sInvalidSlotIndex = (-1);
};

struct GD_RENDER_API FrameGraphResource
{
	using Id = u64;

	Id mId = ~0ULL;
	operator bool() const { return mId != ~0ULL; }
	struct Less
	{
		constexpr bool operator() (const FrameGraphResource& left, const FrameGraphResource& right) const { return left.mId < right.mId; }
	};
};

class GD_RENDER_API FrameGraphMutableResource : public FrameGraphResource
{

};

class GD_RENDER_API ResourceRegistry
{
public:
	ResourceRegistry() {}
	ResourceRegistry(const ResourceRegistry&) = delete;
	ResourceRegistry& operator=(const ResourceRegistry&) = delete;

	FrameGraphMutableResource	CreateTransientResource(const GI::MemoryResourceDesc& desc);
	FrameGraphMutableResource	ImportResource(GI::IGraphicMemoryResource* resource);
	GI::IGraphicMemoryResource* GetResource(const FrameGraphResource& resource) const;
	GI::MemoryResourceDesc		GetResourceDesc(const FrameGraphResource& resource) const;

	void						OnSubmitPass(GI::IGraphicsInfra* infra);
	void						OnEndFrame();

protected:
	std::map<FrameGraphResource::Id, GI::MemoryResourceDesc> mTransienceResourceDescs;
	std::map<FrameGraphResource::Id, std::unique_ptr<GI::IGraphicMemoryResource>> mTransienceResources;

	BijectionMap<FrameGraphResource::Id, GI::IGraphicMemoryResource*> mImportedResources;

	FrameGraphResource::Id	mResourceIdCounter = 0;
};

#define MUTABLE_RESOURCE_USAGE_FUTURE(Name) \
struct GD_RENDER_API Name##UsageFuture \
{ \
	FrameGraphMutableResource resource; \
	GI::##Name##Desc desc; \
	Name##UsageFuture& operator=(const Name##UsageFuture& o) \
	{ \
		this->resource = o.resource; \
		this->desc = o.desc; \
		return *this; \
	} \
};

#define RESOURCE_USAGE_FUTURE(Name) \
struct GD_RENDER_API Name##UsageFuture \
{ \
	FrameGraphResource resource; \
	GI::##Name##Desc desc; \
	Name##UsageFuture& operator=(const Name##UsageFuture& o) \
	{ \
		this->resource = o.resource; \
		this->desc = o.desc; \
		return *this; \
	} \
};

MUTABLE_RESOURCE_USAGE_FUTURE(Dsv);
MUTABLE_RESOURCE_USAGE_FUTURE(Rtv);
MUTABLE_RESOURCE_USAGE_FUTURE(Uav);
RESOURCE_USAGE_FUTURE(Srv);

class GD_RENDER_API RenderPassBuilder
{
	friend class FrameGraphBuilder;
public:
	RenderPassBuilder(FrameGraphBuilder* builder, const char* passName);

	GI::VbvUsage	Read(const GI::VbvUsage& usage);
	GI::IbvUsage	Read(const GI::IbvUsage& usage);
	GI::SamplerDesc	Read(const GI::SamplerDesc& usage);

	SrvUsageFuture	Read(const SrvUsageFuture& usage) { return Read(usage.resource, usage.desc); }
	SrvUsageFuture	Read(const FrameGraphResource& resource, const GI::SrvDesc& desc);
	UavUsageFuture	Read(const UavUsageFuture& usage) { return Read(usage.resource, usage.desc); }
	UavUsageFuture	Read(const FrameGraphResource& resource, const GI::UavDesc& desc);
	RtvUsageFuture	Write(const RtvUsageFuture& usage) { return Write(usage.resource, usage.desc); }
	RtvUsageFuture	Write(const FrameGraphMutableResource& resource, const GI::RtvDesc& desc);
	DsvUsageFuture	Write(const DsvUsageFuture& usage) { return Write(usage.resource, usage.desc); }
	DsvUsageFuture	Write(const FrameGraphMutableResource& resource, const GI::DsvDesc& desc);
	UavUsageFuture	Write(const UavUsageFuture& usage) { return Write(usage.resource, usage.desc); }
	UavUsageFuture	Write(const FrameGraphMutableResource& resource, const GI::UavDesc& desc);

	void SetPassFunction(std::function<void()> func) { mPassFunction = func; }

protected:
	FrameGraphBuilder* mBuilder;
	std::string mPassName;
	std::vector<FrameGraphResource::Id>	mInputResources;
	std::vector<FrameGraphResource::Id>	mOutputResources;
	std::function<void()>				mPassFunction;
};

class GD_RENDER_API RenderPassResources
{
public:
	RenderPassResources(ResourceRegistry* resourceRegistry);

	GI::SrvUsage	Get(const SrvUsageFuture& usage) const;
	GI::RtvUsage	Get(const RtvUsageFuture& usage) const;
	GI::DsvUsage	Get(const DsvUsageFuture& usage) const;
	GI::UavUsage	Get(const UavUsageFuture& usage) const;

protected:
	ResourceRegistry* mResourceRegistry = nullptr;
};

class GD_RENDER_API FrameGraphBuilder
{
public:
	struct Pass {
		std::string mPassName;
		std::function<void()> mExecute;
	};
	using PassHandle = u32;
	
	void						HandlePassBuilder(const RenderPassBuilder& passBuilder);
	void						MarkOutputNode(const FrameGraphResource& resource);
	void						SubmitPasses();

protected:
	ResourceRegistry* mResourceRegistry = nullptr;

	std::map<FrameGraphResource::Id, DirectedGraph::NodeHandle>	mResourceNodes;
	std::map<DirectedGraph::EdgeHandle, PassHandle>				mPassEdges;
	std::vector<Pass>	mPasses;
	DirectedGraph		mResourceGraph;
	std::set<FrameGraphResource::Id>	mPresentResources;
};

class GD_RENDER_API FrameGraph
{
public:
	FrameGraph(GI::IGraphicsInfra* infra);

	void StartFrame();
	void EndFrame();

	template<typename TPassData>
	void AddPass(
		const char* name,
		std::function<void(RenderPassBuilder& builder, TPassData& data)> setup,
		std::function<void(const TPassData& data, const RenderPassResources& resources, GI::IGraphicsInfra* infra)> execute)
	{
		auto builder = RenderPassBuilder(mFrameGraphBuilder.get(), name);

		TPassData data = {};
		setup(builder, data);
		
		builder.SetPassFunction([data, this, execute] ()
		{
			RenderPassResources resources = { mResourceRegistry.get() };
			execute(data, resources, mInfra);
		});
		
		mFrameGraphBuilder->HandlePassBuilder(builder);
	}

	Blackboard* GetBlackboard() const { return mBlackboard.get(); }

	FrameGraphMutableResource	Create(const GI::MemoryResourceDesc& desc);
	FrameGraphMutableResource	Import(GI::IGraphicMemoryResource* resource);
	void						Present(FrameGraphMutableResource resource);

	GI::MemoryResourceDesc		GetResourceDesc(const FrameGraphResource& resource) const;

private:
	std::unique_ptr<Blackboard>			mBlackboard;
	std::unique_ptr<ResourceRegistry>	mResourceRegistry;
	std::unique_ptr<FrameGraphBuilder>	mFrameGraphBuilder;
	GI::IGraphicsInfra*					mInfra = nullptr;
};

