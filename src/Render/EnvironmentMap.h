#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "FrameGraph.h"

class EnvironmentMap
{
public:
	static std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, SrvUsageFuture> GenerateIrradianceMap(
		FrameGraph* frameGraph, GI::IGraphicsInfra* infra, 
		const SrvUsageFuture& sky, i32 resolution, i32 semiSphereBusbarSampleCount);
	static std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, SrvUsageFuture> GenerateIntegratedBRDF(
		FrameGraph* frameGraph, GI::IGraphicsInfra* infra, i32 resolution);
	static std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, SrvUsageFuture> GeneratePrefilteredEnvironmentMap(
		FrameGraph* frameGraph, GI::IGraphicsInfra* infra, 
		const SrvUsageFuture& src, i32 resolution);

protected:
	static void PrefilterEnvironmentMap(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, 
		const RtvUsageFuture& target, const SrvUsageFuture& src, const Vec2i& targetSize, f32 roughness);
};