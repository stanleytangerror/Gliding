#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "FrameGraph.h"

class EnvironmentMap
{
public:
	static std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, GI::SrvUsage> GenerateIrradianceMap(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, const GI::SrvUsage& sky, i32 resolution, i32 semiSphereBusbarSampleCount);
	static std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, GI::SrvUsage> GenerateIntegratedBRDF(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, i32 resolution);
	static std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, GI::SrvUsage> GeneratePrefilteredEnvironmentMap(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, const GI::SrvUsage& src, i32 resolution);

protected:
	static void PrefilterEnvironmentMap(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, const GI::RtvUsage& target, const GI::SrvUsage& src, const Vec2i& targetSize, f32 roughness);
};