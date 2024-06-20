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

	NodeHandle AddNode();
	EdgeHandle AddEdge(const NodeHandle& begin, const NodeHandle& end);

	void RemoveEdge(const EdgeHandle& edge);
	void RemoveNode(const NodeHandle& node);

	std::vector<EdgeHandle>	GetIncomingEdges(const NodeHandle& node) const;
	std::vector<EdgeHandle>	GetOutgoingEdges(const NodeHandle& node) const;
	std::vector<NodeHandle>	GetIncomingNodes(const NodeHandle& node) const;
	std::vector<NodeHandle>	GetOutgoingNodes(const NodeHandle& node) const;

	Edge GetEdge(const EdgeHandle& h) const;

	std::set<NodeHandle>	GetAllNodes() const { return mNodes; }
	std::map<EdgeHandle, Edge>	GetAllEdges() const { return mEdges; }

	static DirectedGraph Cull(const DirectedGraph& graph, const std::vector<NodeHandle>& endNodes);

	static std::vector<NodeHandle> TopoSort(DirectedGraph& graph, const std::vector<NodeHandle>& endNodes);

	static std::string Serialize(const DirectedGraph& graph,
		std::function<std::string(NodeHandle)> serializeNode,
		std::function<std::string(EdgeHandle)> serializeEdge);

protected:
	bool IsValidNodeHandle(const NodeHandle& h) const { return mNodes.find(h) != mNodes.end(); }
	bool IsValidEdgeHandle(const EdgeHandle& h) const { return mEdges.find(h) != mEdges.end(); }

protected:
	std::set<NodeHandle>		mNodes;
	std::map<EdgeHandle, Edge>	mEdges;
	NodeHandle					mNodeCounter = 0;
	EdgeHandle					mEdgeCounter = 0;
};
