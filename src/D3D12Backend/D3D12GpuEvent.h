#pragma once

#include "D3D12Headers.h"

class GD_D3D12BACKEND_API D3D12ScopedEvent
{
public:
	D3D12ScopedEvent(ID3D12GraphicsCommandList* commandList, const char* format);
	~D3D12ScopedEvent();

private:
	ID3D12GraphicsCommandList* const mCommandList = nullptr;
};