#pragma once

#include "D3D12/D3D12Headers.h"

class D3D12ScopedEvent
{
public:
	D3D12ScopedEvent(ID3D12GraphicsCommandList* commandList, const char* format);
	~D3D12ScopedEvent();

private:
	ID3D12GraphicsCommandList* const mCommandList = nullptr;
};