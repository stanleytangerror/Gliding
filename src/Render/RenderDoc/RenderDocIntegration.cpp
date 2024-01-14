#include "RenderPch.h"
#include "RenderDocIntegration.h"

const char* RenderDocIntegration::DllName = "renderdoc.dll";

const char* RenderDocIntegration::CaptureFolderPath = "temp/capture";

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
				mApi->SetCaptureFilePathTemplate(CaptureFolderPath);
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

void RenderDocIntegration::OnStartFrame(GI::DevicePtr device, PortHandle windowHandle)
{
	if (!mApi) { return; }

	switch (mCaptureState)
	{
	case eWaitForNextFrame:
	{
		mCaptureState = eCapturingThisFrame;

		mApi->StartFrameCapture(device, RENDERDOC_WindowHandle(windowHandle));
	}
	break;
	default:
		break;
	}
}

void RenderDocIntegration::OnEndFrame(GI::DevicePtr device, PortHandle windowHandle)
{
	if (!mApi) { return; }

	switch (mCaptureState)
	{
	case eCapturingThisFrame:
	{
		if (mApi->EndFrameCapture(device, RENDERDOC_WindowHandle(windowHandle)))
		{
			char LogFile[512] = {};
			uint64_t Timestamp;
			uint32_t LogPathLength = 512;
			uint32_t Index = 0;
			while (mApi->GetCapture(Index, LogFile, &LogPathLength, &Timestamp))
			{
				Index++;
				DEBUG_PRINT("RenderDoc capture No.%d to %s", Index, LogFile);

				if (!mApi->IsTargetControlConnected())
				{
					mApi->LaunchReplayUI(1, nullptr);
				}
			}
		}
		mCaptureState = eInactive;
	}
	break;

	default:
		break;
	}
}

