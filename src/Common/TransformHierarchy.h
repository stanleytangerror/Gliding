#pragma once

#include "CommonMath.h"
#include <functional>

template <typename T>
struct TransformNode
{
	T								mContent = {};

	Transformf						mRelTransform = Transformf::Identity();
	Transformf						mAbsTransform = Transformf::Identity();

	std::vector<std::unique_ptr<TransformNode<T>>>	mChildren;
	TransformNode<T>*				mParent = nullptr;

	TransformNode<T>* PushChild(const T& content, const Transformf& relTransform = Transformf::Identity())
	{
		mChildren.push_back(std::make_unique<TransformNode<T>>());

		const auto& node = mChildren.back();
		node->mContent = content;
		node->mParent = this;
		node->mRelTransform = relTransform;

		return node.get();
	}

	TransformNode<T>* PushChild(T&& content, const Transformf& relTransform = Transformf::Identity())
	{
		mChildren.push_back(std::make_unique<TransformNode<T>>());

		const auto& node = mChildren.back();
		std::swap(node->mContent, content);
		node->mParent = this;
		node->mRelTransform = relTransform;

		return node.get();
	}

	void CalcAbsTransform()
	{
		ForEach(
			[](TransformNode<T>& node)
			{
				node.mAbsTransform = node.mParent ? 
					node.mParent->mAbsTransform * node.mRelTransform :
					node.mRelTransform;
			});
	}

	void ForEach(const std::function<void(TransformNode<T>&)>& action)
	{
		action(*this);

		for (const auto& child : mChildren)
		{
			child->ForEach(action);
		}
	}

	void ForEach(const std::function<void(const TransformNode<T>&)>& action) const
	{
		ForEach(action);
	}
};