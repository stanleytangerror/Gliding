#include "CommonPch.h"
#include "DirectedGraph.h"
#include "StringUtils.h"
#include "Profile.h"

DirectedGraph::NodeHandle DirectedGraph::AddNode()
{
	auto n = mNodeCounter++;
	if (n >= mNodes.size())
	{
		mNodes.resize(std::max(16llu, mNodes.size() * 2));
	}

	mNodes[n].mValid = true;
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

	const auto& nodeEdges = mNodes[node];
	for (auto n : nodeEdges.mIncomingNodes)
	{
		mNodes[n].mOutgoingNodes.erase(node);
	}
	for (auto n : nodeEdges.mOutgoingNodes)
	{
		mNodes[n].mIncomingNodes.erase(node);
	}
	
	mNodes[node].mValid = false;
	mNodes[node].mIncomingNodes.clear();
	mNodes[node].mOutgoingNodes.clear();
}

u32 DirectedGraph::GetInDegree(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes[node].mIncomingNodes.size();
}

u32 DirectedGraph::GetOutDegree(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes[node].mOutgoingNodes.size();
}

const std::unordered_set<DirectedGraph::NodeHandle>&	DirectedGraph::GetIncomingNodesRef(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes[node].mIncomingNodes;
}

const std::unordered_set<DirectedGraph::NodeHandle>&	DirectedGraph::GetOutgoingNodesRef(const NodeHandle& node) const
{
	Assert(IsValidNodeHandle(node));
	return mNodes[node].mOutgoingNodes;
}

void DirectedGraph::ForEachNodes(std::function<void(NodeHandle, const Node&)> action) const
{
	for (auto n = 0; n < std::min<u32>(mNodes.size(), mNodeCounter); ++n)
	{
		if (IsValidNodeHandle(n))
		{
			action(n, mNodes[n]);
		}
	}
}

DirectedGraph DirectedGraph::Cull(const DirectedGraph& graph, const std::vector<DirectedGraph::NodeHandle>& endNodes)
{
	PROFILE_EVENT(DirectedGraph::Cull);

	DirectedGraph result = graph;

	std::queue<NodeHandle> nodes;
	std::vector<bool> visitedNodes(graph.mNodeCounter, false);
	for (auto n : endNodes) 
	{ 
		nodes.push(n);
		visitedNodes[n] = true;
	}

	{
		PROFILE_EVENT(DirectedGraph::Visit);

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
		PROFILE_EVENT(DirectedGraph::Clean);

		std::vector<NodeHandle> cullingNodes;
		result.ForEachNodes([&](NodeHandle n, const Node& node)
			{
				if (visitedNodes[n] == false)
				{
					cullingNodes.push_back(n);
				}
			});
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

		const auto incomingNodesCopy = graph.GetIncomingNodesRef(curNode);
		graph.RemoveNode(curNode);

		for (auto n : incomingNodesCopy)
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
	graph.ForEachNodes([&](NodeHandle n, const Node& node)
		{
			auto [value, group] = serializeNode(n);
			const auto& str = Utils::FormatString(
				R"({"id":"%d", "value":"%s", "group":"%s"})",
				n, value.c_str(), group.c_str());
			nodeStrs.push_back(str);
		});

	std::vector<std::string> edgeStrs;
	graph.ForEachNodes([&](NodeHandle n, const Node& node)
		{
			for (const auto& o : node.mOutgoingNodes)
			{
				const auto& str = Utils::FormatString(
					R"({"source":"%d", "target":"%d"})",
					n, o);
				edgeStrs.push_back(str);
			}
		});

	std::string result = R"({ "nodes": [)";
	result += joinStrs(",", nodeStrs);
	result += R"(], "edges": [)";
	result += joinStrs(",", edgeStrs);
	result += R"(] })";

	return result;
}

bool DirectedGraph::IsValidNodeHandle(const NodeHandle& h) const
{
	return h < mNodeCounter && h < mNodes.size() && mNodes[h].mValid;
}
