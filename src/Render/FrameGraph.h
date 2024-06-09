#pragma once

#include "Common/GraphicsInfrastructure.h"

class FrameGraph
{
public:
	FrameGraph(GI::IGraphicsInfra* infra);

	template<typename PassData>
	void AddPass(
		const char* name,
		std::function<void(PassData& data)> setup,
		std::function<void(const PassData& data, GI::IGraphicsInfra* infra)> execute)
	{
		PassData data = {};
		setup(data);
		execute(data, mInfra);
	}

private:
	GI::IGraphicsInfra* mInfra = nullptr;
};