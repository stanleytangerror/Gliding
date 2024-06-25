#pragma once

#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include "CommonTypes.h"
#include "AssertUtils.h"

struct GD_COMMON_API DirectedGraph
{
public:
	using NodeHandle = u32;

	struct Node
	{
		std::set<NodeHandle> mIncomingNodes;
		std::set<NodeHandle> mOutgoingNodes;
	};

	NodeHandle AddNode();
	void AddEdge(const NodeHandle& begin, const NodeHandle& end);

	void RemoveNode(const NodeHandle& node);

	u32						GetInDegree(const NodeHandle& node) const;
	u32						GetOutDegree(const NodeHandle& node) const;
	std::set<NodeHandle>	GetIncomingNodes(const NodeHandle& node) const;
	std::set<NodeHandle>	GetOutgoingNodes(const NodeHandle& node) const;

	std::map<NodeHandle, Node>	GetAllNodes() const { return mNodes; }

	static DirectedGraph Cull(const DirectedGraph& graph, const std::vector<NodeHandle>& endNodes);

	static std::vector<NodeHandle> TopoSort(DirectedGraph& graph, const std::vector<NodeHandle>& endNodes);

	static std::string Serialize(const DirectedGraph& graph,
		std::function<std::tuple<std::string, std::string>(NodeHandle)> serializeNode);

protected:
	bool IsValidNodeHandle(const NodeHandle& h) const { return mNodes.find(h) != mNodes.end(); }

protected:
	std::map<NodeHandle, Node>	mNodes;
	NodeHandle					mNodeCounter = 0;
};
