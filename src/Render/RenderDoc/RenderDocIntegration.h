#pragma once

#include "renderdoc_app.h"
#include "Common/GraphicsInfrastructure.h"
#include "Common/PresentPort.h"

class RenderDocApi;

class RenderDocIntegration
{
public:
	RenderDocIntegration();

	bool CaptureNextFrame();

	void OnStartFrame(GI::DevicePtr device, PortHandle windowHandle);
	void OnEndFrame(GI::DevicePtr device, PortHandle windowHandle);

private:
	using RenderDocApi = RENDERDOC_API_1_0_0;
	enum CaptureState { eInactive, eWaitForNextFrame, eCapturingThisFrame };
	static const char*	DllName;
	static const char*	CaptureFolderPath;

	RenderDocApi* mApi = nullptr;
	CaptureState				mCaptureState = eInactive;
};