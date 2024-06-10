#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "Common/CommonUtils.h"

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
	const u64 mId = ~0ULL;
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
	FrameGraphMutableResource	CreateTransientResource(const GI::MemoryResourceDesc& desc);
	FrameGraphMutableResource	ImportResource(GI::IGraphicMemoryResource* resource);

protected:
	std::map<u64, GI::MemoryResourceDesc> mTransienceResources;
	std::map<u64, GI::IGraphicMemoryResource*> mImportedResources;
	u64	mResourceIdCounter = 0;
};

class GD_RENDER_API RenderPassBuilder
{
public:
	RenderPassBuilder(const char* passName);

	GI::VbvUsage	Read(const GI::VbvUsage& usage);
	GI::IbvUsage	Read(const GI::IbvUsage& usage);
	GI::SrvUsage	Read(const GI::SrvUsage& usage);
	GI::SrvUsage	Read(GI::IGraphicMemoryResource* resource, const GI::SrvDesc& desc);
	GI::SamplerDesc	Read(const GI::SamplerDesc& usage);
	GI::UavUsage	Write(const GI::UavUsage& usage);
	GI::RtvUsage	Write(const GI::RtvUsage& usage);
	GI::RtvUsage	Write(GI::IGraphicMemoryResource* resource, const GI::RtvDesc& desc);
	GI::DsvUsage	Write(const GI::DsvUsage& usage);

protected:
	const std::string	mPassName;
};

class GD_RENDER_API FrameGraph
{
public:
	FrameGraph(GI::IGraphicsInfra* infra);

	template<typename TPassData>
	void AddPass(
		const char* name,
		std::function<void(RenderPassBuilder& builder, TPassData& data)> setup,
		std::function<void(const TPassData& data, GI::IGraphicsInfra* infra)> execute)
	{
		auto builder = new RenderPassBuilder(name);

		TPassData data = {};
		setup(*builder, data);


		execute(data, mInfra);
	}

	Blackboard* GetBlackboard() const { return mBlackboard.get(); }

	ResourceRegistry* GetResourceRegistry() const { return mResourceRegistry.get(); }

private:
	std::unique_ptr<Blackboard>			mBlackboard;
	std::unique_ptr<ResourceRegistry>	mResourceRegistry;
	//std::unique_ptr<RenderPassBuilder>	mRenderPassBuilder;
	GI::IGraphicsInfra*					mInfra = nullptr;
};

