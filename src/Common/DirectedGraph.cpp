#include "CommonPch.h"
#include "DirectedGraph.h"
#include "StringUtils.h"
#include "Profile.h"

DirectedGraph::NodeHandle DirectedGraph::AddNode()
{
	auto n = mNodeCounter++;
	mNodes.insert({ n, Node{} });
	return n;
}

DirectedGraph::EdgeHandle DirectedGraph::AddEdge(const NodeHandle& begin, const NodeHandle& end)
{
	Assert(IsValidNodeHandle(begin));
	Assert(IsValidNodeHandle(end));

	auto e = mEdgeCounter++;
	mEdges[e] = { begin, end };
	mNodes[begin].mOutgoingEdges.insert(e);
	mNodes[end].mIncomingEdges.insert(e);
	return e;
}

void DirectedGraph::RemoveEdge(const EdgeHandle& edge)
{
	Assert(IsValidEdgeHandle(edge));

	auto be = mEdges.find(edge)->second;
	Assert(IsValidNodeHandle(be.mBegin));
	Assert(IsValidNodeHandle(be.mEnd));

	mEdges.erase(mEdges.find(edge));
	mNodes[be.mBegin].mOutgoingEdges.erase(edge);
	mNodes[be.mEnd].mIncomingEdges.erase(edge);
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
	
	auto n = mNodes.find(node)->second;
	Assert(n.mIncomingEdges.empty());
	Assert(n.mOutgoingEdges.empty());
	mNodes.erase(node);
}

u32 DirectedGraph::GetInDegree(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes.find(node)->second.mIncomingEdges.size();
}

u32 DirectedGraph::GetOutDegree(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes.find(node)->second.mOutgoingEdges.size();
}

std::set<DirectedGraph::EdgeHandle>	DirectedGraph::GetIncomingEdges(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes.find(node)->second.mIncomingEdges;
}

std::set<DirectedGraph::EdgeHandle>	DirectedGraph::GetOutgoingEdges(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes.find(node)->second.mOutgoingEdges;
}


std::set<DirectedGraph::NodeHandle>	DirectedGraph::GetIncomingNodes(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));

	std::set<NodeHandle> result;
	for (const auto& e : GetIncomingEdges(node))
	{
		result.insert(GetEdge(e).mBegin);
	}
	return result;
}

std::set<DirectedGraph::NodeHandle>	DirectedGraph::GetOutgoingNodes(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));

	std::set<NodeHandle> result;
	for (const auto& e : GetOutgoingEdges(node))
	{
		result.insert(GetEdge(e).mEnd);
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
	PROFILE_EVENT(DirectedGraph::Cull);

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

	std::vector<NodeHandle> cullingNodes;
	for (const auto& [n, _] : result.GetAllNodes())
	{
		if (visitedNodes.find(n) == visitedNodes.end())
		{
			cullingNodes.push_back(n);
		}
	}
	for (const auto& n : cullingNodes)
	{
		result.RemoveNode(n);
	}

	struct EdgeComparer {
		bool operator() (const Edge& lhs, const Edge& rhs) const {
			return 
				lhs.mBegin < rhs.mBegin ? true :
				lhs.mBegin > rhs.mBegin ? false :
				lhs.mEnd < rhs.mEnd;
		}
	};

	bool continu = true;
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
	PROFILE_EVENT(DirectedGraph::TopoSort);

	std::vector<DirectedGraph::NodeHandle> result;

	std::queue<NodeHandle> nodes;
	for (auto n : endNodes) 
	{
		Assert(graph.GetOutDegree(n) == 0);
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
			if (graph.GetOutDegree(n) == 0)
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
		if (it != graph.mNodes.begin()) result += ",";

		auto [value, group] = serializeNode(it->first);
		
		result += Utils::FormatString(
			R"({"id":"%d", "value":"%s", "group":"%s"})", 
			it->first, value.c_str(), group.c_str());
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
