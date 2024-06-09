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

class GD_RENDER_API FrameGraph
{
public:
	FrameGraph(GI::IGraphicsInfra* infra);

	template<typename TPassData>
	void AddPass(
		const char* name,
		std::function<void(TPassData& data)> setup,
		std::function<void(const TPassData& data, GI::IGraphicsInfra* infra)> execute)
	{
		TPassData data = {};
		setup(data);
		execute(data, mInfra);
	}

	Blackboard* GetBlackboard() const { return mBlackboard.get(); }

private:
	std::unique_ptr<Blackboard>		mBlackboard;
	GI::IGraphicsInfra* mInfra = nullptr;
};