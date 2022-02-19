#pragma once

#include "Common/Math.h"

class ID3D12Resource
{
public:
	virtual void Transition(ID3D12GraphicsCommandList* commandList, const D3D12_RESOURCE_STATES& destState) = 0;
	virtual ID3D12Resource* GetD3D12Resource() const = 0;
	virtual Vec3f GetSize() const = 0;
};

class SwapChainBufferResource : public ID3D12Resource
{
public:
	SwapChainBufferResource(ID3D12Resource* res);

	ID3D12Resource* GetD3D12Resource() const override { return mResource; }
	Vec3s				GetSize() const override { return { m_width, m_height, 0 }; }

	RTV* GetRtv() const { return mRtv; }
	void Transition(ID3D12GraphicsCommandList* commandList, const D3D12_RESOURCE_STATES& destState) override;

protected:
	ID3D12Resource* const	mResource = nullptr;
	D3D12_RESOURCE_STATES	mResStates = D3D12_RESOURCE_STATE_COMMON;
	RTV* mRtv = nullptr;

public:
	int32_t					m_width = 0;
	int32_t					m_height = 0;
};