#include "RenderPch.h"
#include "D3D12RenderTarget.h"


D3D12RenderTarget::D3D12RenderTarget(D3D12Device* device, Vec3i size, DXGI_FORMAT format, const char* name)
	: mDevice(device)
	, mSize(size)
	, mFormat(format)
	, mState(D3D12_RESOURCE_STATE_RENDER_TARGET)
{
	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	{
		textureDesc.MipLevels = 1;
		textureDesc.Format = mFormat;
		textureDesc.Width = mSize.x();
		textureDesc.Height = mSize.y();
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		textureDesc.DepthOrArraySize = mSize.z();
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	}

	// create gpu resource default as copy dest
	const CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT };
	AssertHResultOk(device->GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		mState,
		nullptr,
		IID_PPV_ARGS(&mResource)));

	NAME_RAW_D3D12_OBJECT(mResource, name);

	mSrv = new SRV(mDevice, this);
	mRtv = CreateTex2DRtv();
	mUav = new UAV(mDevice, this);
}

void D3D12RenderTarget::Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState)
{
	if (mState != destState)
	{
		context->Transition(mResource, mState, destState);
		mState = destState;
	}
}

RTV* D3D12RenderTarget::CreateTex2DRtv()
{
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	{
		desc.Format = mFormat;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		desc.Texture2D.PlaneSlice = 0;
	}

	return new RTV(mDevice, this, desc);
}

RTV* D3D12RenderTarget::CreateTex2DArrayRtv(uint32_t firstArrayIdx, uint32_t arrayCount)
{
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	{
		desc.Format = mFormat;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipSlice = 0;
		desc.Texture2DArray.FirstArraySlice = firstArrayIdx;
		desc.Texture2DArray.ArraySize = arrayCount;
		desc.Texture2DArray.PlaneSlice = 0;
	}

	return new RTV(mDevice, this, desc);
}

SRV* D3D12RenderTarget::CreateTexCubeSrv()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	{
		desc.Format = mFormat;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.TextureCube.MipLevels = -1;
		desc.TextureCube.ResourceMinLODClamp = 0.f;
		desc.TextureCube.MostDetailedMip = 0;
	}

	return new SRV(mDevice, this, desc);
}

//DSV* D3D12RenderTarget::CreateTex2DDsv()
//{
//	return new DSV(this);
//}

void D3D12RenderTarget::Clear(D3D12CommandContext* context, const Vec4f& color)
{
	Transition(context, D3D12_RESOURCE_STATE_RENDER_TARGET);

	using F4 = const FLOAT[4];
	context->GetCommandList()->ClearRenderTargetView(GetRtv()->GetHandle(), *(F4*)&color, 0, nullptr);
}
