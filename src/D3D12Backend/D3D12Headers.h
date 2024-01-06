#pragma once

#include "directx/d3d12.h"
#include "directx/d3dx12.h"
#include "directx/dxgicommon.h"
#include "directx/dxgiformat.h"
#include "d3dcompiler.h"
#include "dxgi.h"
#include "dxgiformat.h"
#include "windows.h"
#include <wrl/client.h>
#include <dxgi1_4.h>

namespace D3D12Backend
{
	enum D3D12GpuQueueType : u8
	{
		Graphic, Compute, Copy, Count
	};
}

#include "D3D12Utils.h"
#include "D3D12GpuEvent.h"

#if 0
#define RENDER_EVENT(context, format)\
	D3D12Backend::D3D12ScopedEvent _D3D12ScopedEvent_##_FILE_##_LINE_NO_(context->GetCommandList(), #format);\
	Profile::ScopedCpuEvent _Profile_ScopedCpuEvent_##_FILE_##_LINE_NO_(#format);
#else
#define RENDER_EVENT(context, format) ()
#endif

