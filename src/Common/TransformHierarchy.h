#pragma once

#include "Math.h"
#include <functional>

template <typename T>
struct TransformNode
{
	T								mContent = {};

	Transformf						mRelTransform = Transformf::Identity();
	Transformf						mAbsTransform = Transformf::Identity();

	std::vector<TransformNode<T>>	mChildren;
	TransformNode<T>*				mParent = nullptr;

	//void CalcAbsTransform(const Transformf& parentAbsTrans)
	//{
	//	mAbsTransform = parentAbsTrans * mRelTransform;

	//	OutputDebugString(Math::ToString(mRelTransform.matrix()).c_str());
	//	OutputDebugString(Math::ToString(mAbsTransform.matrix()).c_str());
	//	for (TransformNode<T>& child : mChildren)
	//	{
	//		child.CalcAbsTransform(mAbsTransform);
	//	}
	//}

	void PushChild(const T& content, const Transformf& relTransform = Transformf::Identity())
	{
		mChildren.push_back({});
		auto& node = mChildren.back();
		node.mContent = content;
		node.mParent = this;
		node.mRelTransform = relTransform;
	}

	void PushChild(T&& content, const Transformf& relTransform = Transformf::Identity())
	{
		mChildren.push_back({});
		auto& node = mChildren.back();
		node.mContent = std::forward<T>(content);
		node.mParent = this;
		node.mRelTransform = relTransform;
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

		for (TransformNode<T>& child : mChildren)
		{
			child.ForEach(action);
		}
	}

	void ForEach(const std::function<void(const TransformNode<T>&)>& action) const
	{
		ForEach(action);
	}
};