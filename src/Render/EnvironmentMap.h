#pragma once

#include "Common/GraphicsInfrastructure.h"

class EnvironmentMap
{
public:
	static std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, GI::SrvDesc> GenerateIrradianceMap(GI::IGraphicsInfra* infra, const GI::SrvDesc& sky, i32 resolution, i32 semiSphereBusbarSampleCount);
	static std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, GI::SrvDesc> GenerateIntegratedBRDF(GI::IGraphicsInfra* infra, i32 resolution);
	static std::tuple<std::unique_ptr<GI::IGraphicMemoryResource>, GI::SrvDesc> GeneratePrefilteredEnvironmentMap(GI::IGraphicsInfra* infra, const GI::SrvDesc& src, i32 resolution);

protected:
	static void PrefilterEnvironmentMap(GI::IGraphicsInfra* infra, const GI::RtvDesc& target, const GI::SrvDesc& src, const Vec2i& targetSize, f32 roughness);
};