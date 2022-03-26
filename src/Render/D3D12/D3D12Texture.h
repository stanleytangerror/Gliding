#pragma once

#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12Resource.h"
#include "D3D12/D3D12ResourceView.h"
#include "D3D12/D3D12CommandContext.h"

class D3D12Texture : public ID3D12Res
{
public:
	D3D12Texture(D3D12Device* device, const char* filePath);
	D3D12Texture(D3D12Device* device, const char* filePath, const std::vector<b8>& content);
	virtual ~D3D12Texture();

	void							Initial(D3D12CommandContext* context);

	ID3D12Resource*					GetD3D12Resource() const override { return mD3D12Resource; }
	Vec3i							GetSize() const override { return mSize; }

	bool							IsD3DResourceReady() const { return mD3D12Resource; }

	D3D12_RESOURCE_STATES			GetResStates() const { return mResStates; }
	SRV*							GetSrv() const { return mSrv; }

	void							Transition(D3D12CommandContext* context, const D3D12_RESOURCE_STATES& destState) override;

protected:
	std::vector<b8>	mContent;

	D3D12Device* const		mDevice = nullptr;
	std::string				mFilePath;
	ID3D12Resource*			mD3D12Resource = nullptr;
	D3D12_RESOURCE_STATES	mResStates = D3D12_RESOURCE_STATE_COPY_DEST;
	Vec3i					mSize = {};

	SRV* mSrv = nullptr;
	int32_t					mWidth = 0;
	int32_t					mHeight = 0;
};
