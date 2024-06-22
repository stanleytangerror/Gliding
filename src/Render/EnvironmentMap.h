#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "FrameGraph.h"

class EnvironmentMap
{
public:
	static SrvUsageFuture GenerateIrradianceMap(
		FrameGraph* frameGraph, GI::IGraphicsInfra* infra, 
		const FrameGraphResource& sky, i32 resolution, i32 semiSphereBusbarSampleCount);
	static SrvUsageFuture GenerateIntegratedBRDF(
		FrameGraph* frameGraph, GI::IGraphicsInfra* infra, i32 resolution);
	static SrvUsageFuture GeneratePrefilteredEnvironmentMap(
		FrameGraph* frameGraph, GI::IGraphicsInfra* infra, 
		const FrameGraphResource& src, i32 resolution);

protected:
	static void PrefilterEnvironmentMap(FrameGraph* frameGraph, GI::IGraphicsInfra* infra, 
		FrameGraphMutableResource targetResource, GI::RtvDesc& targetDesc, 
		const FrameGraphResource& src, const GI::SrvDesc& srcDesc,
		const Vec2i& targetSize, f32 roughness);
};