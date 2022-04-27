#include "D3D12BackendPch.h"
#include "D3D12GpuEvent.h"
//#include "WinPixEventRuntime.1.0.220124001/Include/WinPixEventRuntime/pix3.h"

D3D12ScopedEvent::D3D12ScopedEvent(ID3D12GraphicsCommandList* commandList, const char* format)
	: mCommandList(commandList)
{
#if defined(_PIX_H_) || defined(_PIX3_H_)
	//PIXBeginEvent(mCommandList, 0, format);
#endif
}

D3D12ScopedEvent::~D3D12ScopedEvent()
{
#if defined(_PIX_H_) || defined(_PIX3_H_)
	//PIXEndEvent(mCommandList);
#endif
}