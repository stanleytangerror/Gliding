#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "Common/CommonUtils.h"
#include "Common/Container.h"
#include "Common/DirectedGraph.h"

class RenderPassResources;

template <typename T>
struct BaseId {
	u16 index = InvalidIndex;
	inline static u16 InvalidIndex = ~0;

	constexpr bool isValid() const { return index != InvalidIndex; }
	constexpr bool operator==(const T& other) const { return index == other.index; }
	constexpr bool operator!=(const T& other) const { return index != other.index; }
	constexpr bool operator<(const T& other) const { return index < other.index; }

	constexpr static T create(u16 idx) { T result; result.index = idx; return result; }
};

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
	struct Id
	{
		struct Handle : BaseId<Handle> {};
		struct Version : BaseId<Version> {};
		
		Handle mHandle;
		Version mVersion;

		Id() {}
		Id(u16 idx) : mHandle(Handle::create(idx)), mVersion(Version::create(0)) {}

		constexpr bool IsValid() const { return mHandle.isValid() && mVersion.isValid(); }
		constexpr bool operator==(const Id& other) const { return (mHandle == other.mHandle) && (mVersion == other.mVersion); }
		constexpr bool operator!=(const Id& other) const { return (*this).operator==(other); }
		constexpr bool operator<(const Id& other) const { 
			return mHandle < other.mHandle ? true :
					mHandle == other.mHandle ? mVersion < other.mVersion : false; }

		std::string	GetDebugName() const { return Utils::FormatString("%d.%d", mHandle.index, mVersion.index); }
	};

	Id mId;

	constexpr bool IsValid() const { return mId.IsValid(); }

	struct Less
	{
		constexpr bool operator() (const FrameGraphResource& left, const FrameGraphResource& right) const { return left.mId < right.mId; }
	};
};

class GD_RENDER_API FrameGraphMutableResource : public FrameGraphResource
{
public:
	void IncrementVersion() 
	{ 
		mId.mVersion = Id::Version::create(mId.mVersion.index + 1);
	}
};

class GD_RENDER_API ResourceRegistry
{
public:
	ResourceRegistry() {}
	ResourceRegistry(const ResourceRegistry&) = delete;
	ResourceRegistry& operator=(const ResourceRegistry&) = delete;

	FrameGraphMutableResource	CreatePermanentResource(const GI::MemoryResourceDesc& desc);
	FrameGraphMutableResource	CreateTransientResource(const GI::MemoryResourceDesc& desc);
	FrameGraphMutableResource	ImportResource(GI::IGraphicMemoryResource* resource);
	GI::IGraphicMemoryResource* GetResource(const FrameGraphResource& resource) const;
	GI::MemoryResourceDesc		GetResourceDesc(const FrameGraphResource& resource) const;

	void						OnCompile(GI::IGraphicsInfra* infra);
	void						OnEndFrame();

protected:
	struct ResourceData
	{
		GI::MemoryResourceDesc						mDesc;
		std::unique_ptr<GI::IGraphicMemoryResource>	mRealResource;
	};

	std::map<FrameGraphResource::Id::Handle, ResourceData> mPermanentResources;
	std::map<FrameGraphResource::Id::Handle, ResourceData> mTransienceResources;

	BijectionMap<FrameGraphResource::Id::Handle, GI::IGraphicMemoryResource*> mImportedResources;

	u16							mResourceIdCounter = 0;
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

	FrameGraphResource	Read(const FrameGraphResource& resource);

	SrvUsageFuture	ReadTex2DSrv(const FrameGraphResource& resource);
	SrvUsageFuture	ReadBufferSrv(const FrameGraphResource& resource, u32 numElements, u32 stride);
	SrvUsageFuture	Read(const FrameGraphResource& resource, const GI::SrvDesc& desc);
	
	UavUsageFuture	Read(const FrameGraphResource& resource, const GI::UavDesc& desc);
	
	RtvUsageFuture	WriteTex2DRtv(FrameGraphMutableResource& resource);
	RtvUsageFuture	Write(FrameGraphMutableResource& resource, const GI::RtvDesc& desc);
	
	DsvUsageFuture	WriteTex2DDsv(FrameGraphMutableResource& resource);
	DsvUsageFuture	Write(FrameGraphMutableResource& resource, const GI::DsvDesc& desc);
	
	UavUsageFuture	WriteBufferUav(FrameGraphMutableResource& resource, u32 numElements, u32 stride);
	UavUsageFuture	Write(FrameGraphMutableResource& resource, const GI::UavDesc& desc);
	FrameGraphMutableResource	Write(FrameGraphMutableResource& resource);

	DsvUsageFuture	ReadWriteTex2DDsv(FrameGraphMutableResource& resource);
	DsvUsageFuture	ReadWrite(FrameGraphMutableResource& resource, const GI::DsvDesc& desc);

	UavUsageFuture	ReadWriteTex2DUav(FrameGraphMutableResource& resource);
	UavUsageFuture	ReadWrite(FrameGraphMutableResource& resource, const GI::UavDesc& desc);

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

	GI::IGraphicMemoryResource*	Get(const FrameGraphResource::Id& resource) const;
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
	FrameGraphBuilder(ResourceRegistry* registry);

	ResourceRegistry*			GetResourceRegistry() const { return mResourceRegistry; }

	void						HandlePassBuilder(const RenderPassBuilder& passBuilder);
	void						MarkOutputNode(const FrameGraphResource& resource);
	void						CompileAndExecute();

	void						DebugOutputGraph();
	void						DebugOutputResourceNode(DirectedGraph::NodeHandle node, const char* prefix = nullptr);
	void						DebugOutputPassNode(DirectedGraph::EdgeHandle edge, const char* prefix = nullptr);

protected:
	struct Pass {
		std::string mPassName;
		std::function<void()> mExecute;
	};
	using PassHandle = u32;

protected:
	ResourceRegistry* mResourceRegistry = nullptr;

	BijectionMap<FrameGraphResource::Id, DirectedGraph::NodeHandle>	mResourceNodes;
	BijectionMap<PassHandle, DirectedGraph::NodeHandle>				mPassNodes;
	DirectedGraph													mResourceGraph;
	
	std::set<FrameGraphResource::Id>								mPresentResources;
	std::vector<Pass>												mPasses;
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

	FrameGraphMutableResource	CreatePermanent(const GI::MemoryResourceDesc& desc);
	FrameGraphMutableResource	CreateTransient(const GI::MemoryResourceDesc& desc);
	FrameGraphMutableResource	Import(GI::IGraphicMemoryResource* resource);
	void						Present(FrameGraphMutableResource resource);

	GI::MemoryResourceDesc		GetResourceDesc(const FrameGraphResource& resource) const;

private:
	std::unique_ptr<Blackboard>			mBlackboard;
	std::unique_ptr<ResourceRegistry>	mResourceRegistry;
	std::unique_ptr<FrameGraphBuilder>	mFrameGraphBuilder;
	GI::IGraphicsInfra*					mInfra = nullptr;
};

