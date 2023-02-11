#pragma once

#include "RenderInterface/RenderTypes.h"
#include "D3D12Backend/D3D12Headers.h"

constexpr DXGI_FORMAT ToDxgiFormat(RHI::PixelFormat format) { return DXGI_FORMAT(format); }

constexpr D3D12_INPUT_CLASSIFICATION ToInputClassification(RHI::InputClassification c) { return D3D12_INPUT_CLASSIFICATION(c); }

constexpr D3D12_RESOURCE_DIMENSION ToResourceDimension(RHI::ResourceDimention d) { return D3D12_RESOURCE_DIMENSION(d); }

constexpr D3D12_RESOURCE_FLAGS ToResourceFlags(u32 f) { return D3D12_RESOURCE_FLAGS(f); }

constexpr D3D12_FILTER ToFilter(RHI::FilterType f) { return D3D12_FILTER(f); }

constexpr D3D12_TEXTURE_ADDRESS_MODE ToRextureAddressMode(RHI::TextureAddressMode m) { return D3D12_TEXTURE_ADDRESS_MODE(m); }

constexpr D3D12_COMPARISON_FUNC ToComparisonFunc(RHI::ComparisonFunc f) { return D3D12_COMPARISON_FUNC(f); }

inline D3D12_INPUT_ELEMENT_DESC ToInputElementDesc(const RHI::InputElementDesc& desc)
{
	return D3D12_INPUT_ELEMENT_DESC
	{
		desc.SemanticName.c_str(),
		desc.SemanticIndex,
		ToDxgiFormat(desc.Format),
		desc.Slot,
		desc.AlignedByteOffset,
		ToInputClassification(desc.InputSlotClass),
		desc.InstanceDataStepRate
	};
}

inline D3D12_VIEWPORT ToViewPort(const RHI::ViewPort& viewPort)
{
	return CD3DX12_VIEWPORT(
		viewPort.LeftTop.x(), viewPort.LeftTop.y(), 
		viewPort.Size.x(), viewPort.Size.y(),
		viewPort.DepthRange.x(), viewPort.DepthRange.y());
}

inline D3D12_RECT ToRect(const RHI::Rect& rect)
{
	return CD3DX12_RECT(
		rect.LeftTop.x(), rect.LeftTop.y(),
		rect.RightBottom.x(), rect.RightBottom.y());
}

constexpr D3D12_SRV_DIMENSION ToSrvDimension(RHI::ViewDimension d)
{
	switch (d)
	{
	case RHI::ViewDimension::BUFFER							  : return D3D12_SRV_DIMENSION_BUFFER							 ;
	case RHI::ViewDimension::TEXTURE1D						  : return D3D12_SRV_DIMENSION_TEXTURE1D						 ;
	case RHI::ViewDimension::TEXTURE1DARRAY					  : return D3D12_SRV_DIMENSION_TEXTURE1DARRAY					 ;
	case RHI::ViewDimension::TEXTURE2D						  : return D3D12_SRV_DIMENSION_TEXTURE2D						 ;
	case RHI::ViewDimension::TEXTURE2DARRAY					  : return D3D12_SRV_DIMENSION_TEXTURE2DARRAY					 ;
	case RHI::ViewDimension::TEXTURE2DMS					  : return D3D12_SRV_DIMENSION_TEXTURE2DMS						 ;
	case RHI::ViewDimension::TEXTURE2DMSARRAY				  : return D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY				 ;
	case RHI::ViewDimension::TEXTURE3D						  : return D3D12_SRV_DIMENSION_TEXTURE3D						 ;
	case RHI::ViewDimension::TEXTURECUBE					  : return D3D12_SRV_DIMENSION_TEXTURECUBE						 ;
	case RHI::ViewDimension::TEXTURECUBEARRAY				  : return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY				 ;
	case RHI::ViewDimension::RAYTRACING_ACCELERATION_STRUCTURE: return D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	default													  : return D3D12_SRV_DIMENSION_UNKNOWN							 ;
	}
}

constexpr D3D12_RTV_DIMENSION ToRtvDimension(RHI::ViewDimension d)
{
	switch (d)
	{
	case RHI::ViewDimension::BUFFER							  : return D3D12_RTV_DIMENSION_BUFFER							 ;
	case RHI::ViewDimension::TEXTURE1D						  : return D3D12_RTV_DIMENSION_TEXTURE1D						 ;
	case RHI::ViewDimension::TEXTURE1DARRAY					  : return D3D12_RTV_DIMENSION_TEXTURE1DARRAY					 ;
	case RHI::ViewDimension::TEXTURE2D						  : return D3D12_RTV_DIMENSION_TEXTURE2D						 ;
	case RHI::ViewDimension::TEXTURE2DARRAY					  : return D3D12_RTV_DIMENSION_TEXTURE2DARRAY					 ;
	case RHI::ViewDimension::TEXTURE2DMS					  : return D3D12_RTV_DIMENSION_TEXTURE2DMS						 ;
	case RHI::ViewDimension::TEXTURE2DMSARRAY				  : return D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY					 ;
	case RHI::ViewDimension::TEXTURE3D						  : return D3D12_RTV_DIMENSION_TEXTURE3D						 ;
	default													  : return D3D12_RTV_DIMENSION_UNKNOWN							 ;
	}
}

constexpr D3D12_BUFFER_SRV ToSrvDesc(const RHI::SRV::BufferViewDesc& desc)
{
	return D3D12_BUFFER_SRV{ desc.FirstElement, desc.NumElements, desc.StructureByteStride, (D3D12_BUFFER_SRV_FLAGS)desc.Flags };
}
constexpr D3D12_TEX1D_SRV ToSrvDesc(const RHI::SRV::Texture1DView& desc)
{
	return D3D12_TEX1D_SRV{ desc.MostDetailedMip, desc.MipLevels, desc.MinLODClamp };
}
constexpr D3D12_TEX1D_ARRAY_SRV ToSrvDesc(const RHI::SRV::Texture1DArrayView& desc)
{
	return D3D12_TEX1D_ARRAY_SRV{ desc.MostDetailedMip, desc.MipLevels, desc.FirstArraySlice, desc.ArraySize, desc.MinLODClamp };
}
constexpr D3D12_TEX2D_SRV ToSrvDesc(const RHI::SRV::Texture2DView& desc)
{
	return D3D12_TEX2D_SRV{ desc.MostDetailedMip, desc.MipLevels, desc.PlaneSlice, desc.MinLODClamp };
}
constexpr D3D12_TEX2D_ARRAY_SRV ToSrvDesc(const RHI::SRV::Texture2DArrayView& desc)
{
	return D3D12_TEX2D_ARRAY_SRV{ desc.MostDetailedMip, desc.MipLevels, desc.FirstArraySlice, desc.ArraySize, desc.PlaneSlice, desc.MinLODClamp };
}
constexpr D3D12_TEX2DMS_SRV ToSrvDesc(const RHI::SRV::Texture2DMSView& desc)
{
	return D3D12_TEX2DMS_SRV{ 0 };
}
constexpr D3D12_TEX2DMS_ARRAY_SRV ToSrvDesc(const RHI::SRV::Texture2DMSArrayView& desc)
{
	return D3D12_TEX2DMS_ARRAY_SRV{ desc.FirstArraySlice, desc.ArraySize };
}
constexpr D3D12_TEX3D_SRV ToSrvDesc(const RHI::SRV::Texture3DView& desc)
{
	return D3D12_TEX3D_SRV{ desc.MostDetailedMip, desc.MipLevels, desc.MinLODClamp };
}
constexpr D3D12_TEXCUBE_SRV ToSrvDesc(const RHI::SRV::TextureCubeView& desc)
{
	return D3D12_TEXCUBE_SRV{ desc.MostDetailedMip, desc.MipLevels, desc.MinLODClamp };
}
constexpr D3D12_TEXCUBE_ARRAY_SRV ToSrvDesc(const RHI::SRV::TextureCubeArrayView& desc)
{
	return D3D12_TEXCUBE_ARRAY_SRV{ desc.MostDetailedMip, desc.MipLevels, desc.First2DArrayFace, desc.NumCubes, desc.MinLODClamp };
}
constexpr D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV ToSrvDesc(const RHI::SRV::RaytracingAccelerationStructureView& desc)
{
	return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV{ desc.GpuVirtualAddress };
}


constexpr D3D12_BUFFER_RTV ToRtvDesc(const RHI::RTV::BufferViewDesc& desc)
{
	return D3D12_BUFFER_RTV{ desc.FirstElement, desc.NumElements };
}
constexpr D3D12_TEX1D_RTV ToRtvDesc(const RHI::RTV::Texture1DView& desc)
{
	return D3D12_TEX1D_RTV{ desc.MipSlice };
}
constexpr D3D12_TEX1D_ARRAY_RTV ToRtvDesc(const RHI::RTV::Texture1DArrayView& desc)
{
	return D3D12_TEX1D_ARRAY_RTV{ desc.MipSlice, desc.FirstArraySlice, desc.ArraySize };
}
constexpr D3D12_TEX2D_RTV ToRtvDesc(const RHI::RTV::Texture2DView& desc)
{
	return D3D12_TEX2D_RTV{ desc.MipSlice, desc.PlaneSlice };
}
constexpr D3D12_TEX2D_ARRAY_RTV ToRtvDesc(const RHI::RTV::Texture2DArrayView& desc)
{
	return D3D12_TEX2D_ARRAY_RTV{ desc.MipSlice, desc.FirstArraySlice, desc.ArraySize, desc.PlaneSlice };
}
constexpr D3D12_TEX2DMS_RTV ToRtvDesc(const RHI::RTV::Texture2DMSView& desc)
{
	return D3D12_TEX2DMS_RTV{ 0 };
}
constexpr D3D12_TEX2DMS_ARRAY_RTV ToRtvDesc(const RHI::RTV::Texture2DMSArrayView& desc)
{
	return D3D12_TEX2DMS_ARRAY_RTV{ desc.FirstArraySlice, desc.ArraySize };
}
constexpr D3D12_TEX3D_RTV ToRtvDesc(const RHI::RTV::Texture3DView& desc)
{
	return D3D12_TEX3D_RTV{ desc.MipSlice, desc.FirstWSlice, desc.WSize };
}
