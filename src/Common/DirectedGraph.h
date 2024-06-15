#pragma once

#include <vector>
#include <queue>
#include <algorithm>
#include "CommonTypes.h"
#include "AssertUtils.h"

struct GD_COMMON_API DirectedGraph
{
public:
	using NodeHandle = u32;
	using EdgeHandle = u32;

	struct Edge
	{
		NodeHandle mBegin;
		NodeHandle mEnd;
	};

	NodeHandle AddNode()
	{
		auto n = mNodeCounter++;
		mNodes.insert(n);
		return n;
	}

	void TryAddNode(NodeHandle n)
	{
		if (!IsValidNodeHandle(n))
		{
			mNodeCounter = std::max(mNodeCounter, n + 1);
			mNodes.insert(n);
		}
	}

	EdgeHandle AddEdge(const NodeHandle& begin, const NodeHandle& end)
	{
		Assert(IsValidNodeHandle(begin));
		Assert(IsValidNodeHandle(end));

		auto e = mEdgeCounter++;
		mEdges[e] = { begin, end };
		return e;
	}

	void TryAddEdge(const NodeHandle& begin, const NodeHandle& end, const EdgeHandle& edge)
	{
		Assert(IsValidNodeHandle(begin));
		Assert(IsValidNodeHandle(end));
		if (IsValidEdgeHandle(edge))
		{
			auto e = GetEdge(edge);
			Assert(e.mBegin == begin);
			Assert(e.mEnd == end);
		}

		mEdgeCounter = std::max(mEdgeCounter, edge + 1);
		mEdges[edge] = { begin, end };
	}

	void RemoveEdge(const EdgeHandle& edge)
	{
		Assert(IsValidEdgeHandle(edge));

		mEdges.erase(mEdges.find(edge));
	}
	
	void RemoveNode(const NodeHandle& node)
	{
		Assert(IsValidNodeHandle(node));

		for (auto e : GetIncomingEdges(node))
		{
			RemoveEdge(e);
		}
		for (auto e : GetOutgoingEdges(node))
		{
			RemoveEdge(e);
		}
	}

	std::vector<EdgeHandle>	GetIncomingEdges(const NodeHandle& node) const
	{
		std::vector<EdgeHandle> result;
		for (const auto& [e, n] : mEdges)
		{
			if (n.mEnd == node)
			{
				result.push_back(e);
			}
		}
		return result;
	}

	std::vector<EdgeHandle>	GetOutgoingEdges(const NodeHandle& node) const
	{
		std::vector<EdgeHandle> result;
		for (const auto& [e, n] : mEdges)
		{
			if (n.mBegin == node)
			{
				result.push_back(e);
			}
		}
		return result;
	}

	std::vector<NodeHandle>	GetIncomingNodes(const NodeHandle& node) const
	{
		std::vector<NodeHandle> result;
		for (const auto& [e, n] : mEdges)
		{
			if (n.mEnd == node)
			{
				result.push_back(n.mBegin);
			}
		}
		return result;
	}

	std::vector<NodeHandle>	GetOutgoingNodes(const NodeHandle& node) const
	{
		std::vector<NodeHandle> result;
		for (const auto& [e, n] : mEdges)
		{
			if (n.mBegin == node)
			{
				result.push_back(n.mEnd);
			}
		}
		return result;
	}

	static DirectedGraph Cull(const DirectedGraph& graph, const std::vector<NodeHandle>& endNodes);

	static std::vector<NodeHandle> CullAndSort(const DirectedGraph& graph, const std::vector<NodeHandle>& endNodes);

protected:
	bool IsValidNodeHandle(const NodeHandle& h) const { return mNodes.find(h) != mNodes.end(); }
	bool IsValidEdgeHandle(const EdgeHandle& h) const { return mEdges.find(h) != mEdges.end(); }
	
	Edge GetEdge(const EdgeHandle& h) const
	{
		Assert(IsValidEdgeHandle(h));
		return mEdges.find(h)->second;
	}

protected:
	std::set<NodeHandle>		mNodes;
	std::map<EdgeHandle, Edge>	mEdges;
	NodeHandle					mNodeCounter = 0;
	EdgeHandle					mEdgeCounter = 0;
};
