#include "CommonPch.h"
#include "GraphicsInfrastructure.h"

namespace GI
{
	RtvDesc ResouceViewUtils::CreateFullRtv(const IGraphicMemoryResource* resource, Format::Enum rtvFormat, u32 mipSlice, u32 planeSlice)
	{
		switch (resource->GetDimension())
		{
		case GI::ResourceDimension::BUFFER:

		case GI::ResourceDimension::TEXTURE1D:
		case GI::ResourceDimension::TEXTURE2D:
			return GI::RtvDesc()
				.SetResource(resource)
				.SetFormat(rtvFormat)
				.SetViewDimension(GI::RtvDimension::TEXTURE2D)
				.SetTexture2D_MipSlice(mipSlice)
				.SetTexture2D_PlaneSlice(planeSlice);
		case GI::ResourceDimension::TEXTURE3D:
			break;
		}

		return GI::RtvDesc();
	}
}