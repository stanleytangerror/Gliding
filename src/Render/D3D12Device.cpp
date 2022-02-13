#include "D3D12Device.h"
#include "Common/Assert.h"
#include "D3D12/D3D12Utils.h"

namespace
{
	static int FrameCount = 2;
	static int Width = 1280;
	static int Height = 720;
}

void D3D12Device::Initial(HWND windowHandle)
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
			IID_PPV_ARGS(&m_device)
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
			IID_PPV_ARGS(&m_device)
		);

		AssertHResultOk(hr);
	}

	// create info queue
#if defined(_DEBUG)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
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

	AssertHResultOk(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

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
		m_commandQueue,		// Swap chain needs the queue so that it can force a flush on it.
		windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	//Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
	//AssertHResultOk(swapChain1.As(&swapChain3));
	m_swapChain = reinterpret_cast<IDXGISwapChain3*>(swapChain1);

	mCommandAllocatorPool = new D3D12Device::CommandAllocatorPool(
		[&]()
		{
			ID3D12CommandAllocator* newAllocator = nullptr;
			Assert(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&newAllocator)));
			return newAllocator;
		}, 
		[](ID3D12CommandAllocator* a) { a->Reset(); },
		[](ID3D12CommandAllocator* a) { a->Release(); });
}
