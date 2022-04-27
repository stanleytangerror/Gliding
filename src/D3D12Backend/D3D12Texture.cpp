#include "D3D12BackendPch.h"
#include "D3D12Texture.h"

D3D12Texture::D3D12Texture(D3D12Device* device, const char* filePath)
	: mDevice(device)
	, mFilePath(filePath)
{
	mContent = Utils::LoadFileContent(filePath);
}

D3D12Texture::D3D12Texture(D3D12Device* device, const char* filePath, const std::vector<b8>& content)
	: mDevice(device)
	, mFilePath(filePath)
	, mContent(content)
{

}

D3D12Texture::~D3D12Texture()
{
	mDevice->ReleaseD3D12Resource(mD3D12Resource);
}

void D3D12Texture::Initial(D3D12CommandContext* context)
{
	mD3D12Resource = D3D12Utils::CreateTextureFromImageMemory(context, mFilePath.c_str(), mContent);

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
