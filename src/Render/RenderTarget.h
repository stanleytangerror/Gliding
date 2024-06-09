#pragma once

#include "Common/GraphicsInfrastructure.h"

class GD_RENDER_API RenderTarget
{
public:
	RenderTarget(GI::IGraphicsInfra* infra, Vec3u size, GI::Format::Enum format, const char* name);
	RenderTarget(GI::IGraphicsInfra* infra, Vec3u size, GI::Format::Enum format, i32 mipLevelCount, const char* name);
	RenderTarget(GI::IGraphicsInfra* infra, i32 count, i32 stride, GI::Format::Enum format, const char* name);

	GI::RtvUsage			GetRtv() const { return mRtv; }
	GI::UavUsage			GetUav() const { return mUav; }
	GI::SrvUsage			GetSrv() const { return mSrv; }

protected:
	std::unique_ptr<GI::IGraphicMemoryResource> mResource;
	Vec3u						mSize = {};
	i32							mMipLevelCount = 1;
	GI::Format::Enum			mFormat;

	GI::RtvUsage					mRtv;
	GI::UavUsage					mUav;
	GI::SrvUsage					mSrv;
};
