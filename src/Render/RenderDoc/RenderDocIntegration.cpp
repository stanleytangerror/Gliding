#include "RenderPch.h"
#include "RenderDocIntegration.h"

const char* RenderDocIntegration::DllName = "renderdoc.dll";

RenderDocIntegration::RenderDocIntegration()
{
	if (const HMODULE mod = LoadLibraryEx(DllName, nullptr, 0))
	{
		if (pRENDERDOC_GetAPI getApi = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI"))
		{
			RENDERDOC_API_1_0_0* rdoc = nullptr;
			if (getApi(eRENDERDOC_API_Version_1_0_0, (void**)& rdoc) == 1)
			{
				mApi = rdoc;
			}
		}
	}
}

bool RenderDocIntegration::CaptureNextFrame()
{
	if (!mApi) { return false; }

	switch (mCaptureState)
	{
	case eInactive:
		mCaptureState = eWaitForNextFrame;
		return true;
	default:
		return false;
	}
}

void RenderDocIntegration::OnStartFrame(D3D12Device* device, HWND windowHandle)
{
	if (!mApi) { return; }

	switch (mCaptureState)
	{
	case eWaitForNextFrame:
	{
		mCaptureState = eCapturingThisFrame;

		mApi->StartFrameCapture(device->GetDevice(), windowHandle);
	}
	break;
	default:
		break;
	}
}

void RenderDocIntegration::OnEndFrame(D3D12Device* device, HWND windowHandle)
{
	if (!mApi) { return; }

	switch (mCaptureState)
	{
	case eCapturingThisFrame:
	{
		if (mApi->EndFrameCapture(device->GetDevice(), windowHandle))
		{
			char LogFile[512] = {};
			uint64_t Timestamp;
			uint32_t LogPathLength = 512;
			uint32_t Index = 0;
			while (mApi->GetCapture(Index, LogFile, &LogPathLength, &Timestamp))
			{
				Index++;
				DEBUG_PRINT("RenderDoc capture No.%d to %s", Index, LogFile);
			}
		}
		mCaptureState = eInactive;
	}
	break;

	default:
		break;
	}
}

