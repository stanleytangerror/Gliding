#include "D3D12BackendPch.h"
#include "D3D12Device.h"
#include "D3D12Utils.h"
#include "D3D12CommandContext.h"
#include "D3D12SwapChain.h"
#include "D3D12ResourceManager.h"

#if defined(_DEBUG)
#define ENABLE_D3D12_DEBUG_LAYER 1
#define ENABLE_D3D12_DEBUG_LAYER_BREAK_ON_ERROR 1
#else
#define ENABLE_D3D12_DEBUG_LAYER 0
#define ENABLE_D3D12_DEBUG_LAYER_BREAK_ON_ERROR 0
#endif

namespace D3D12Backend
{
	D3D12Device::D3D12Device()
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

		AssertHResultOk(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));

		if (false)
		{
			Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
			AssertHResultOk(mFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

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
			D3D12Utils::GetHardwareAdapter(mFactory, &hardwareAdapter);

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

		mResMgr = std::make_unique<ResourceManager>(this);

		for (u8 i = 0; i < u8(D3D12GpuQueueType::Count); ++i)
		{
			mGpuQueues[i] = new D3D12GpuQueue(this, D3D12GpuQueueType(i), Utils::FormatString("GPU queue %s", 
				i == D3D12GpuQueueType::Graphic ? "Graphic" :
				i == D3D12GpuQueueType::Compute ? "Compute" :
				i == D3D12GpuQueueType::Copy ? "Copy" : "Unknown"
			).c_str());
		}

		mPipelineStateLib = new D3D12PipelineStateLibrary(this);
		mShaderLib = new D3D12ShaderLibrary;

		mNullSrvCpuDesc = mResMgr->CreateSrvDescriptor({},
			GI::SrvDesc()
			.SetViewDimension(GI::SrvDimension::TEXTURE2D)
			.SetFormat(GI::Format::FORMAT_R8G8B8A8_UNORM)
			.SetTexture2D_MipLevels(1)
			.SetTexture2D_MostDetailedMip(0));

		mNullSamplerCpuDesc = mResMgr->CreateSampler(GI::SamplerDesc()
			.SetFilter(GI::Filter::MIN_MAG_MIP_POINT)
			.SetAddress({ GI::TextureAddressMode::WRAP,  GI::TextureAddressMode::WRAP,  GI::TextureAddressMode::WRAP }));
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
		PROFILE_EVENT(Present);

		for (D3D12GpuQueue* q : mGpuQueues)
		{
			q->Execute();
		}

		for (auto swapChain : mResMgr->GetSwapChains())
		{
			swapChain->Present();
		}

		for (D3D12GpuQueue* q : mGpuQueues)
		{
			q->CpuWaitForThisQueue(q->GetGpuPlannedValue() >= 2 ? q->GetGpuPlannedValue() - 2 : 0);
		}

		mResMgr->Update();

		bool postSyncQueueNotEmpty = std::any_of(mPostSyncQueues.begin(), mPostSyncQueues.end(), [](const auto& q) { return !q.empty(); });
		if (postSyncQueueNotEmpty)
		{
			while (!mPostSyncQueues[PreRelease].empty())
			{
				mPostSyncQueues[PreRelease].front()();
				mPostSyncQueues[PreRelease].pop();
			}

			for (D3D12GpuQueue* q : mGpuQueues)
			{
				q->CpuWaitForThisQueue(q->GetGpuPlannedValue());
			}
			mResMgr->Update();

			while (!mPostSyncQueues[PostRelease].empty())
			{
				mPostSyncQueues[PostRelease].front()();
				mPostSyncQueues[PostRelease].pop();
			}
		}
	}

	void D3D12Device::Destroy()
	{
		for (D3D12GpuQueue*& q : mGpuQueues)
		{
			q->CpuWaitForThisQueue(q->GetGpuPlannedValue());
		}

		mResMgr->Update();
		mResMgr = nullptr;

		for (D3D12GpuQueue*& q : mGpuQueues)
		{
			Utils::SafeDelete(q);
		}

		Utils::SafeDelete(mPipelineStateLib);
		Utils::SafeDelete(mShaderLib);

		Utils::SafeRelease(mDevice);
	}

	ID3D12Device* D3D12Device::GetDevice() const
	{
		return mDevice;
	}


	IDXGIFactory4* D3D12Device::GetFactory() const
	{
		return mFactory;
	}

	void D3D12Device::PushPostSyncOperation(D3D12Device::PostSyncStage stage, const PostSyncOperation& operation)
	{
		mPostSyncQueues[stage].push(operation);
	}

}