#pragma once

#include <vector>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include "CommonTypes.h"
#include "AssertUtils.h"

struct GD_COMMON_API DirectedGraph
{
public:
	using NodeHandle = u32;

	struct Node
	{
		bool mValid = false;
		std::unordered_set<NodeHandle> mIncomingNodes;
		std::unordered_set<NodeHandle> mOutgoingNodes;
	};

	NodeHandle AddNode();
	void AddEdge(const NodeHandle& begin, const NodeHandle& end);

	void RemoveNode(const NodeHandle& node);

	u32						GetInDegree(const NodeHandle& node) const;
	u32						GetOutDegree(const NodeHandle& node) const;
	const std::unordered_set<NodeHandle>&	GetIncomingNodesRef(const NodeHandle& node) const;
	const std::unordered_set<NodeHandle>&	GetOutgoingNodesRef(const NodeHandle& node) const;

	const std::vector<Node>& GetAllNodesRef() const { return mNodes; }

	void					ForEachNodes(std::function<void(NodeHandle, const Node&)> action) const;

	static void Cull(DirectedGraph& graph, std::ranges::input_range auto&& reachingNodes)
	{
		std::vector<bool> visitedNodes(graph.mNodeCounter, false);
		{
			std::queue<NodeHandle> nodes;
			for (auto n : reachingNodes)
			{
				nodes.push(n);
				visitedNodes[n] = true;
			}

			while (!nodes.empty())
			{
				auto curNode = nodes.front();
				nodes.pop();

				for (auto n : graph.GetIncomingNodesRef(curNode))
				{
					if (visitedNodes[n] == false)
					{
						nodes.push(n);
						visitedNodes[n] = true;
					}
				}
			}
		}

		{
			std::vector<NodeHandle> cullingNodes;
			graph.ForEachNodes([&](NodeHandle n, const Node& node)
				{
					if (visitedNodes[n] == false)
					{
						cullingNodes.push_back(n);
					}
				});
			for (const auto& n : cullingNodes)
			{
				graph.RemoveNode(n);
			}
		}
	}

	static std::vector<NodeHandle> TopoSort(DirectedGraph& graph);

	static std::string Serialize(const DirectedGraph& graph,
		std::function<std::tuple<std::string, std::string>(NodeHandle)> serializeNode);

protected:
	bool IsValidNodeHandle(const NodeHandle& h) const;

protected:
	std::vector<Node>			mNodes;
	NodeHandle					mNodeCounter = 0;
};
