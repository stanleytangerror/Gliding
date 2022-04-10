#include "RenderPch.h"
#include "D3D12Device.h"
#include "D3D12/D3D12Utils.h"
#include "D3D12/D3D12CommandContext.h"
#include "D3D12/D3D12RenderTarget.h"
#include "D3D12/D3D12SwapChain.h"
#include "D3D12/D3D12ResourceManager.h"

#if defined(_DEBUG)
#define ENABLE_D3D12_DEBUG_LAYER 1
#define ENABLE_D3D12_DEBUG_LAYER_BREAK_ON_ERROR 1
#else
#define ENABLE_D3D12_DEBUG_LAYER 0
#define ENABLE_D3D12_DEBUG_LAYER_BREAK_ON_ERROR 0
#endif

D3D12Device::D3D12Device(HWND windowHandle, const Vec2i& initWindowSize)
{
	UINT dxgiFactoryFlags = 0;

#if ENABLE_D3D12_DEBUG_LAYER
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			debugController->SetEnableGPUBasedValidation(true);

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

#if ENABLE_D3D12_DEBUG_LAYER
	// create info queue
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
#if ENABLE_D3D12_DEBUG_LAYER_BREAK_ON_ERROR
			AssertHResultOk(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
#endif

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

	mResMgr = std::make_unique<D3D12ResourceManager>(this);

	for (u8 i = 0; i < u8(D3D12GpuQueueType::Count); ++i)
	{
		mGpuQueues[i] = new D3D12GpuQueue(this, D3D12GpuQueueType(i));
	}

	mPipelineStateLib = new D3D12PipelineStateLibrary(this);
	mShaderLib = new D3D12ShaderLibrary;

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

	D3D12_SAMPLER_DESC nullSamplerDesc = {};
	{
		nullSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		nullSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		nullSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	mNullSamplerCpuDesc = mDescAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->AllocCpuDesc();
	D3D12Device::GetDevice()->CreateSampler(&nullSamplerDesc, mNullSamplerCpuDesc.Get());

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = mSwapChainBufferCount;
	swapChainDesc.Width = initWindowSize.x();
	swapChainDesc.Height = initWindowSize.y();
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	IDXGISwapChain1* swapChain1 = nullptr;
	AssertHResultOk(factory->CreateSwapChainForHwnd(
		mGpuQueues[D3D12GpuQueueType::Graphic]->GetCommandQueue(),
		windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	mBackBuffers = new SwapChainBuffers(this, reinterpret_cast<IDXGISwapChain3*>(swapChain1), mSwapChainBufferCount);
}

void D3D12Device::StartFrame()
{
	for (D3D12GpuQueue* q : mGpuQueues)
	{
		q->IncreaseGpuPlannedValue(1);
	}
}

void D3D12Device::Present()
{
	for (D3D12GpuQueue* q : mGpuQueues)
	{
		q->Execute();
	}
	
	mBackBuffers->Present();

	for (D3D12GpuQueue* q : mGpuQueues)
	{
		q->CpuWaitForThisQueue(q->GetGpuPlannedValue() >= 1 ? q->GetGpuPlannedValue() - 1 : 0);
	}

	mResMgr->Update();
}

void D3D12Device::Destroy()
{
	for (D3D12GpuQueue*& q : mGpuQueues)
	{
		q->CpuWaitForThisQueue(q->GetGpuPlannedValue());
		Utils::SafeDelete(q);
	}

	Utils::SafeDelete(mBackBuffers);

	mResMgr->Update();
	mResMgr = nullptr;

	//for (D3D12DescriptorAllocator*& alloc : mDescAllocator)
	//{
	//	delete alloc;
	//	alloc = nullptr;
	//}

	Utils::SafeDelete(mPipelineStateLib);
	Utils::SafeDelete(mShaderLib);

	Utils::SafeRelease(mDevice);
}

ID3D12Device* D3D12Device::GetDevice() const
{
	return mDevice;
}

D3D12DescriptorAllocator* D3D12Device::GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	Assert(type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
	return mDescAllocator[type];
}

void D3D12Device::ReleaseD3D12Resource(ID3D12Resource*& res)
{
	mResMgr->ReleaseResource(res);
	res = nullptr;
}
