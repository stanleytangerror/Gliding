#pragma once

#include "D3D12Headers.h"
#include "Common/CommonTypes.h"
#include "Common/Math.h"

class ID3D12Res
{
public:
	virtual void Transition(ID3D12GraphicsCommandList* commandList, const D3D12_RESOURCE_STATES& destState) = 0;
	virtual ID3D12Resource* GetD3D12Resource() const = 0;
	virtual Vec3i GetSize() const = 0;
	virtual std::string GetName() const { return {}; }
};

