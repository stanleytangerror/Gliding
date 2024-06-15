#include "CommonPch.h"
#include "DirectedGraph.h"

DirectedGraph DirectedGraph::Cull(const DirectedGraph& graph, const std::vector<DirectedGraph::NodeHandle>& endNodes)
{
	DirectedGraph result;

	std::queue<NodeHandle> nodes;
	std::set<NodeHandle> visitedNodes;
	for (auto n : endNodes) { nodes.push(n); }

	while (!nodes.empty())
	{
		auto curNode = nodes.front();
		nodes.pop();
		visitedNodes.insert(curNode);

		result.TryAddNode(curNode);

		for (auto e : graph.GetIncomingEdges(curNode))
		{
			auto be = graph.GetEdge(e);
			result.TryAddNode(be.mBegin);
			result.TryAddNode(be.mEnd);
			result.TryAddEdge(be.mBegin, be.mEnd, e);

			if (visitedNodes.find(be.mBegin) == visitedNodes.end())
			{
				nodes.push(be.mBegin);
			}
		}
	}

	return result;
}

std::vector<DirectedGraph::NodeHandle> DirectedGraph::CullAndSort(const DirectedGraph& graph, const std::vector<DirectedGraph::NodeHandle>& endNodes)
{
	DirectedGraph culledGraph = Cull(graph, endNodes);

	std::vector<DirectedGraph::EdgeHandle> result;

	std::queue<NodeHandle> nodes;
	for (auto n : endNodes) { nodes.push(n); }

	while (!nodes.empty())
	{
		auto curNode = nodes.front();
		nodes.pop();

		for (auto e : culledGraph.GetIncomingEdges(curNode))
		{
			result.push_back(e);
		}

		auto nextNodes = culledGraph.GetIncomingNodes(curNode);

		culledGraph.RemoveNode(curNode);
		for (auto n : nextNodes)
		{
			if (culledGraph.GetIncomingEdges(n).empty())
			{
				nodes.push(n);
			}
		}
	}

	std::reverse(result.begin(), result.end());
	return result;
}
