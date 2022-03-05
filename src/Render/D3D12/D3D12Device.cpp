#include "RenderPch.h"
#include "D3D12Device.h"
#include "Common/AssertUtils.h"
#include "D3D12/D3D12Utils.h"
#include "D3D12/D3D12CommandContext.h"

namespace
{
	static int FrameCount = 2;
	static int Width = 1280;
	static int Height = 720;
}

D3D12Device::D3D12Device(HWND windowHandle)
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	//IDXGIFactory4* factory = nullptr;
	Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
	AssertHResultOk(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (false)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
		AssertHResultOk(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		AssertHResultOk(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&mDevice)
		));
	}
	else
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
		//IDXGIAdapter1* hardwareAdapter = nullptr;
		D3D12Utils::GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		DXGI_ADAPTER_DESC desc;
		hardwareAdapter->GetDesc(&desc);

		const HRESULT hr = D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&mDevice)
		);

		AssertHResultOk(hr);
	}

	// create info queue
#if defined(_DEBUG)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			AssertHResultOk(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));

			D3D12_MESSAGE_SEVERITY denySeverities[] =
			{
				D3D12_MESSAGE_SEVERITY_INFO,
				D3D12_MESSAGE_SEVERITY_WARNING,
			};

			D3D12_MESSAGE_ID denyIds[] =
			{
				D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES
			};

			D3D12_INFO_QUEUE_FILTER NewFilter = {};
			NewFilter.DenyList.NumSeverities = _countof(denySeverities);
			NewFilter.DenyList.pSeverityList = denySeverities;
			NewFilter.DenyList.NumIDs = _countof(denyIds);
			NewFilter.DenyList.pIDList = denyIds;
			infoQueue->PushStorageFilter(&NewFilter);

			infoQueue->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_MESSAGE, "InfoQueue initialized");
		}
	}
#endif

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	AssertHResultOk(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = Width;
	swapChainDesc.Height = Height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	IDXGISwapChain1* swapChain1 = nullptr;
	AssertHResultOk(factory->CreateSwapChainForHwnd(
		mCommandQueue,		// Swap chain needs the queue so that it can force a flush on it.
		windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	mPipelineStateLib = new D3D12PipelineStateLibrary(this);
	mShaderLib = new D3D12ShaderLibrary;

	//Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
	//AssertHResultOk(swapChain1.As(&swapChain3));
	mSwapChain = reinterpret_cast<IDXGISwapChain3*>(swapChain1);

	mCommandAllocatorPool = new CommandAllocatorPool(
		[&]()
		{
			ID3D12CommandAllocator* newAllocator = nullptr;
			Assert(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&newAllocator)));
			return newAllocator;
		}, 
		[](ID3D12CommandAllocator* a) { a->Reset(); },
		[](ID3D12CommandAllocator* a) { a->Release(); });

	mFence = new D3D12Fence(mCommandQueue);

	for (i32 i = 0; i < mRuntimeDescHeaps.size(); ++i)
	{
		mRuntimeDescHeaps[i] = new RuntimeDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE(i));
	}

	for (i32 i = 0; i < mDescAllocator.size(); ++i)
	{
		mDescAllocator[i] = new D3D12DescriptorAllocator(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE(i));
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
	{
		nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		nullSrvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		nullSrvDesc.Texture2D.MipLevels = 1;
		nullSrvDesc.Texture2D.MostDetailedMip = 0;
		nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}
	mNullSrvCpuDesc = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->AllocCpuDesc();
	D3D12Device::GetDevice()->CreateShaderResourceView(nullptr, &nullSrvDesc, mNullSrvCpuDesc.Get());

	mGraphicContextPool = new GraphicsContextPool(
		[&]() { return new GraphicsContext(this); },
		[](GraphicsContext* ctx) {},
		[](GraphicsContext* ctx) {}
	);

	mBackBuffers = new SwapChainBuffers(this, mSwapChain, FrameCount);
}

void D3D12Device::Present()
{
	//mCommandQueue->ExecuteCommandLists()

	AssertHResultOk(mSwapChain->Present(1, 0));

	mFence->PlanGpuQueueWork();
	mFence->CpuWaitForGpuQueue();
	mFence->IncreaseCpuFence();
}

ID3D12Device* D3D12Device::GetDevice() const
{
	return mDevice;
}

CommandAllocatorPool* D3D12Device::GetCommandAllocatorPool() const
{
	return mCommandAllocatorPool;
}

RuntimeDescriptorHeap* D3D12Device::GetRuntimeDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	Assert(type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
	return mRuntimeDescHeaps[type];
}

D3D12DescriptorAllocator* D3D12Device::GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	Assert(type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
	return mDescAllocator[type];
}
