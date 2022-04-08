#include "WinLauncherPch.h"
#include "windows.h"
#include <string>
#include "Application.h"

void SetupEnvironment()
{
	char defaultWorkingDir[1024] = {};
	GetCurrentDirectory(1024, defaultWorkingDir);

	HINSTANCE inst = nullptr;
	char exeName[1024] = {};
	if (GetModuleFileName(inst, exeName, sizeof(exeName)))
	{
		const char* offset = "..\\..\\..\\..";

		std::string exeDir = exeName;
		exeDir = exeDir.substr(0, exeDir.find_last_of('\\'));

		std::string workingDir = exeDir + offset;
		const BOOL succ = SetCurrentDirectory(workingDir.c_str());

		char redirectedWorkingDir[1024] = {};
		GetCurrentDirectory(1024, redirectedWorkingDir);

#ifdef _DEBUG
		OutputDebugString("Redirect current directory from [");
		OutputDebugString(defaultWorkingDir);
		OutputDebugString("] to [");
		OutputDebugString(redirectedWorkingDir);
		OutputDebugString("] \n");
#endif // _DEBUG
	}
}

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	SetupEnvironment();

	std::unique_ptr<Application> app = std::make_unique<Application>(960, 540, "TestApplication", hInstance, nCmdShow);
	app->Initial();
	app->Run();
}