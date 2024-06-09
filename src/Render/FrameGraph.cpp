#include "RenderPch.h"
#include "FrameGraph.h"

Blackboard::~Blackboard()
{
	Clear();
}

FrameGraph::FrameGraph(GI::IGraphicsInfra* infra)
	: mInfra(infra)
{
	mBlackboard = std::make_unique<Blackboard>();
}
