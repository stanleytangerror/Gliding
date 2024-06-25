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

void DirectedGraph::AddEdge(const NodeHandle& begin, const NodeHandle& end)
{
	Assert(IsValidNodeHandle(begin));
	Assert(IsValidNodeHandle(end));

	mNodes[begin].mOutgoingNodes.insert(end);
	mNodes[end].mIncomingNodes.insert(begin);
}

void DirectedGraph::RemoveNode(const NodeHandle& node)
{
	Assert(IsValidNodeHandle(node));

	const auto& nodeEdges = mNodes.find(node)->second;
	for (auto n : nodeEdges.mIncomingNodes)
	{
		mNodes[n].mOutgoingNodes.erase(node);
	}
	for (auto n : nodeEdges.mOutgoingNodes)
	{
		mNodes[n].mIncomingNodes.erase(node);
	}
	
	mNodes.erase(node);
}

u32 DirectedGraph::GetInDegree(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes.find(node)->second.mIncomingNodes.size();
}

u32 DirectedGraph::GetOutDegree(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes.find(node)->second.mOutgoingNodes.size();
}

std::set<DirectedGraph::NodeHandle>	DirectedGraph::GetIncomingNodes(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes.find(node)->second.mIncomingNodes;
}

std::set<DirectedGraph::NodeHandle>	DirectedGraph::GetOutgoingNodes(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes.find(node)->second.mOutgoingNodes;
}

DirectedGraph DirectedGraph::Cull(const DirectedGraph& graph, const std::vector<DirectedGraph::NodeHandle>& endNodes)
{
	PROFILE_EVENT(DirectedGraph::Cull);

	DirectedGraph result = graph;

	std::queue<NodeHandle> nodes;
	std::set<NodeHandle> visitedNodes;
	for (auto n : endNodes) 
	{ 
		nodes.push(n);
		visitedNodes.insert(n);
	}

	{
		PROFILE_EVENT(DirectedGraph::Visit);

		while (!nodes.empty())
		{
			auto curNode = nodes.front();
			nodes.pop();

			for (auto n : graph.GetIncomingNodes(curNode))
			{
				if (visitedNodes.find(n) == visitedNodes.end())
				{
					nodes.push(n);
					visitedNodes.insert(n);
				}
			}
		}
	}

	{
		PROFILE_EVENT(DirectedGraph::Clean);

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
	std::function<std::tuple<std::string, std::string>(NodeHandle)> serializeNode)
{
	auto joinStrs = [](const std::string& ch, const std::vector<std::string>& strs)
	{
		std::string result;
		for (auto i = 0; i < strs.size(); ++i)
		{
			result += i != 0 ? ch : "";
			result += strs[i];
		}
		return result;
	};


	std::vector<std::string> nodeStrs;
	for (const auto& [n, h] : graph.mNodes)
	{
		auto [value, group] = serializeNode(n);
		const auto& str = Utils::FormatString(
			R"({"id":"%d", "value":"%s", "group":"%s"})", 
			n, value.c_str(), group.c_str());
		nodeStrs.push_back(str);
	}

	std::vector<std::string> edgeStrs;
	for (const auto& [n, h] : graph.mNodes)
	{
		for (const auto& o : h.mOutgoingNodes)
		{
			const auto& str = Utils::FormatString(
				R"({"source":"%d", "target":"%d"})",
				n, o);
			edgeStrs.push_back(str);
		}
	}

	std::string result = R"({ "nodes": [)";
	result += joinStrs(",", nodeStrs);
	result += R"(], "edges": [)";
	result += joinStrs(",", edgeStrs);
	result += R"(] })";

	return result;
}
