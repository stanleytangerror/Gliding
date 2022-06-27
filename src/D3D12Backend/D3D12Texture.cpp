#include "D3D12BackendPch.h"
#include "D3D12Texture.h"

D3D12Texture::D3D12Texture(D3D12Device* device, const char* filePath, const std::vector<b8>& content)
	: mDevice(device)
	, mName(filePath)
	, mContent(content)
	, mFromImageMemory(true)
{

}

D3D12Texture::D3D12Texture(D3D12Device* device, DXGI_FORMAT format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name)
	: mDevice(device)
	, mName(name)
	, mSize(size)
	, mContent(content)
	, mMipLevelCount(mipLevel)
	, mFormat(format)
	, mFromImageMemory(false)
{

}

D3D12Texture::~D3D12Texture()
{
	mDevice->ReleaseD3D12Resource(mD3D12Resource);
}

void D3D12Texture::Initial(D3D12CommandContext* context)
{
	if (mFromImageMemory)
	{
		const TextureFileExt::Enum ext = Utils::GetTextureExtension(mName.c_str());
		mD3D12Resource = D3D12Utils::CreateTextureFromImageMemory(context, ext, mContent);
	}
	else
	{
		mD3D12Resource = D3D12Utils::CreateTextureFromRawMemory(context, mFormat, mContent, mSize, mMipLevelCount, mName.c_str());
	}

	const auto& desc = mD3D12Resource->GetDesc();
	mSize = { i32(desc.Width), i32(desc.Height), i32(desc.DepthOrArraySize) };
	mFormat = desc.Format;

	NAME_RAW_D3D12_OBJECT(mD3D12Resource, mFilePath.c_str());

	mSrv = new SRV(mDevice, this);
}

void D3D12Texture::Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState)
{
	if (mResStates != destState)
	{
		context->Transition(mD3D12Resource, mResStates, destState);
		mResStates = destState;
	}
}
