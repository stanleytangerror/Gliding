#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "FrameGraph.h"

class EnvironmentMap
{
public:
	static FrameGraphResource GenerateIrradianceMap(
		FrameGraph* frameGraph, 
		const FrameGraphResource& sky, i32 resolution, i32 semiSphereBusbarSampleCount);
	static FrameGraphResource GenerateIntegratedBRDF(
		FrameGraph* frameGraph, i32 resolution);
	static FrameGraphResource GeneratePrefilteredEnvironmentMap(
		FrameGraph* frameGraph, 
		const FrameGraphResource& src, i32 resolution);

protected:
	static void PrefilterEnvironmentMap(FrameGraph* frameGraph, 
		FrameGraphMutableResource& targetResource, GI::RtvDesc& targetDesc, 
		const FrameGraphResource& src, 
		const Vec2i& targetSize, f32 roughness);
};