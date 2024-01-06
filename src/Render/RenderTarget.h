#pragma once

#include "Common/GraphicsInfrastructure.h"

class GD_RENDER_API RenderTarget
{
public:
	RenderTarget(GI::IGraphicsInfra* infra, Vec3i size, GI::Format::Enum format, const char* name);
	RenderTarget(GI::IGraphicsInfra* infra, Vec3i size, GI::Format::Enum format, i32 mipLevelCount, const char* name);
	RenderTarget(GI::IGraphicsInfra* infra, i32 count, i32 stride, GI::Format::Enum format, const char* name);

	GI::RtvDesc			GetRtv() const { return mRtv; }
	GI::UavDesc			GetSrv() const { return mUav; }
	GI::SrvDesc			GetUav() const { return mSrv; }

protected:
	std::unique_ptr<GI::IGraphicMemoryResource> mResource;
	Vec3i						mSize = {};
	i32							mMipLevelCount = 1;
	GI::Format::Enum			mFormat;

	GI::RtvDesc					mRtv;
	GI::UavDesc					mUav;
	GI::SrvDesc					mSrv;
};
