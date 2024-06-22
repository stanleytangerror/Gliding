#include "CommonPch.h"
#include "DirectedGraph.h"
#include "StringUtils.h"

DirectedGraph::NodeHandle DirectedGraph::AddNode()
{
	auto n = mNodeCounter++;
	mNodes.insert(n);
	return n;
}

DirectedGraph::EdgeHandle DirectedGraph::AddEdge(const NodeHandle& begin, const NodeHandle& end)
{
	Assert(IsValidNodeHandle(begin));
	Assert(IsValidNodeHandle(end));

	auto e = mEdgeCounter++;
	mEdges[e] = { begin, end };
	return e;
}

void DirectedGraph::RemoveEdge(const EdgeHandle& edge)
{
	Assert(IsValidEdgeHandle(edge));

	mEdges.erase(mEdges.find(edge));
}

void DirectedGraph::RemoveNode(const NodeHandle& node)
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
	mNodes.erase(node);
}

std::vector<DirectedGraph::EdgeHandle>	DirectedGraph::GetIncomingEdges(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));

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

std::vector<DirectedGraph::EdgeHandle>	DirectedGraph::GetOutgoingEdges(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));

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


std::vector<DirectedGraph::NodeHandle>	DirectedGraph::GetIncomingNodes(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));

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

std::vector<DirectedGraph::NodeHandle>	DirectedGraph::GetOutgoingNodes(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));

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

DirectedGraph::Edge DirectedGraph::GetEdge(const DirectedGraph::EdgeHandle& h) const
{
	Assert(IsValidEdgeHandle(h));
	return mEdges.find(h)->second;
}

DirectedGraph DirectedGraph::Cull(const DirectedGraph& graph, const std::vector<DirectedGraph::NodeHandle>& endNodes)
{
	DirectedGraph result = graph;

	std::queue<NodeHandle> nodes;
	std::set<NodeHandle> visitedNodes;
	for (auto n : endNodes) { nodes.push(n); }

	while (!nodes.empty())
	{
		auto curNode = nodes.front();
		nodes.pop();
		visitedNodes.insert(curNode);

		for (auto n : graph.GetIncomingNodes(curNode))
		{
			if (visitedNodes.find(n) == visitedNodes.end())
			{
				nodes.push(n);
			}
		}
	}

	bool continu = true;
	while (continu)
	{
		continu = false;
		for (auto n : result.GetAllNodes())
		{
			if (visitedNodes.find(n) == visitedNodes.end())
			{
				result.RemoveNode(n);
				continu = true;
				break;
			}
		}
	}

	struct EdgeComparer {
		bool operator() (const Edge& lhs, const Edge& rhs) const {
			return 
				lhs.mBegin < rhs.mBegin ? true :
				lhs.mBegin > rhs.mBegin ? false :
				lhs.mEnd < rhs.mEnd;
		}
	};

	continu = true;
	while (continu)
	{
		continu = false;
		std::set<Edge, EdgeComparer> edges;
		for (auto [eh, e] : result.GetAllEdges())
		{
			if (edges.find(e) != edges.end())
			{
				result.RemoveEdge(eh);
				continu = true;
				break;
			}
			edges.insert(e);
		}
	}

	return result;
}

std::vector<DirectedGraph::NodeHandle> DirectedGraph::TopoSort(DirectedGraph& graph, const std::vector<DirectedGraph::NodeHandle>& endNodes)
{
	std::vector<DirectedGraph::NodeHandle> result;

	std::queue<NodeHandle> nodes;
	for (auto n : endNodes) 
	{
		Assert(graph.GetOutgoingNodes(n).empty());
		nodes.push(n); 
	}

	while (!nodes.empty())
	{
		auto curNode = nodes.front();
		nodes.pop();

		result.push_back(curNode);

		const auto& incomingNodes = graph.GetIncomingNodes(curNode);
		graph.RemoveNode(curNode);

		for (auto n : incomingNodes)
		{
			if (graph.GetOutgoingNodes(n).empty())
			{
				nodes.push(n);
			}
		}
	}

	std::reverse(result.begin(), result.end());
	return result;
}

std::string DirectedGraph::Serialize(const DirectedGraph& graph,
	std::function<std::tuple<std::string, std::string>(NodeHandle)> serializeNode,
	std::function<std::string(EdgeHandle)> serializeEdge)
{
	std::string result = R"({ "nodes": [)";
	for (auto it = graph.mNodes.begin(); it != graph.mNodes.end(); ++it)
	{
		auto n = *it;
		if (it != graph.mNodes.begin()) result += ",";
		auto [value, group] = serializeNode(n);
		result += Utils::FormatString(
			R"({"id":"%d", "value":"%s", "group":"%s"})", 
			n, value.c_str(), group.c_str());
	}
	result += R"(], "edges": [)";
	for (auto it = graph.mEdges.begin(); it != graph.mEdges.end(); ++it)
	{
		if (it != graph.mEdges.begin()) result += ",";
		result += Utils::FormatString(
			R"({"source":"%d", "target":"%d", "value":"%s"})", 
			it->second.mBegin, it->second.mEnd, serializeEdge(it->first).c_str());
	}
	result += R"(] })";
	return result;
}
