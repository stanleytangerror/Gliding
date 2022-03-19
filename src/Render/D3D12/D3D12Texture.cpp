#include "RenderPch.h"
#include "D3D12Texture.h"

D3D12Texture::D3D12Texture(D3D12Device* device, const char* filePath)
	: mDevice(device)
	, mFilePath(filePath)
{
	mImage = D3D12Utils::LoadDDSImageFromFile(filePath);

	const auto& metadata = mImage->GetMetadata();
	mSize = { i32(metadata.width), i32(metadata.height), i32(metadata.depth) };
}

D3D12Texture::~D3D12Texture()
{
	mDevice->ReleaseD3D12Resource(mD3D12Resource);
}

void D3D12Texture::Initial(D3D12CommandContext* context)
{
	const auto& result = D3D12Utils::CreateD3DResFromDDSImage(mDevice->GetDevice(), context->GetCommandList(), *mImage);
	mD3D12Resource = result.first;
	ID3D12Resource* tempRes = result.second;
	NAME_RAW_D3D12_OBJECT(mD3D12Resource, mFilePath.c_str());
	NAME_RAW_D3D12_OBJECT(tempRes, "IntermediateHeap");
	mDevice->ReleaseD3D12Resource(tempRes);

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
