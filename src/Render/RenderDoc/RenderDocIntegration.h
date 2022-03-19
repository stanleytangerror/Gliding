#pragma once

#include "renderdoc_app.h"
#include "D3D12/D3D12Headers.h"
#include "D3D12/D3D12Device.h"

class D3D12Device;
class RenderDocApi;

class RenderDocIntegration
{
public:
	RenderDocIntegration();

	bool CaptureNextFrame();

	void OnStartFrame(D3D12Device* device, HWND windowHandle);
	void OnEndFrame(D3D12Device* device, HWND windowHandle);

private:
	using RenderDocApi = RENDERDOC_API_1_0_0;
	enum CaptureState { eInactive, eWaitForNextFrame, eCapturingThisFrame };
	static const char*	DllName;
	static const char*	CaptureFolderPath;

	RenderDocApi* mApi = nullptr;
	CaptureState				mCaptureState = eInactive;
};