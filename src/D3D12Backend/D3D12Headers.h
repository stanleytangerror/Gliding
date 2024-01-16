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

#include "Common/GraphicsInfrastructure.h"
#include "D3D12Utils.h"

