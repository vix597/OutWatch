#include <Windows.h>
#include <iostream>
#include <Shlwapi.h>
#include <sstream>
#include <thread>

#include "injector.h"
#include "process_util.h"
#include "status.h"

using namespace std;

#define PIPE_SERVER_NAME "\\\\.\\pipe\\StJudeOutput"
#define BUFFER_SIZE 32768

#pragma comment(lib,"Shlwapi.lib")

#define INJECT_DLL L".\\OutputInterceptor.dll"
static wchar_t * g_appName = NULL;
static wchar_t g_modulePath[MAX_PATH] = { L'\0' };
static wchar_t * g_moduleName = NULL;
static bool g_appRunning = true;
static thread g_receiveOutputThread;
static Injector injector;

void PrintUsage()
{
	cout << "Usage: " << g_appName << " -p <process_name>\n";
}

bool ConsoleHandler(int s)
{
	if (s == CTRL_C_EVENT){
		g_appRunning = false;
		CancelSynchronousIo((HANDLE)g_receiveOutputThread.native_handle());
	}
	return true;
}

void ReceiveOutput();

int wmain(int argc, wchar_t * argv[])
{
	g_appName = argv[0];
	int pid = 0;
	DWORD modulePathLen = 0;
	Status status;

	if (argc != 3){
		PrintUsage();
		return 1;
	}

	if (!lstrcmpW(argv[1], L"-p")){
		status = GetProcessIdByName(argv[2], &pid);
		if (!status.success){
			cerr << "Error: " << status.errorString << " " << status.errorCode << "\n";
			return 1;
		}
	}
	else{
		PrintUsage();
		return 1;
	}

	//Get full path to module (assuming it's in the current directory)
	modulePathLen = GetFullPathNameW(INJECT_DLL, MAX_PATH, g_modulePath, &g_moduleName);
	if (!modulePathLen){
		wcerr << "Cannot get full path name for: " << INJECT_DLL << "\n";
		return 1;
	}

	if (!PathFileExistsW(g_modulePath)){
		wcerr << "File does not exist: " << g_modulePath << "\n";
		return 1;
	}

	if (!(status = injector.Attach(pid)).success){
		cerr << "Process attach failed: " << status.errorString << " " << status.errorCode << "\n";
		return 1;
	}
	if (!(status = injector.InjectModule(g_modulePath)).success){
		cerr << "Inject failed: " << status.errorString << " " << status.errorCode << "\n";
		injector.Detach();
		return 1;
	}
	
	if (!(status = injector.CallRemoteProc(g_modulePath, g_moduleName, "Init", NULL, 0)).success){
		cerr << "Init() failed: " << status.errorString << " " << status.errorCode << "\n";
		injector.UnloadModule(g_moduleName);
		injector.Detach();
		return 1;
	}
	cout << "Thread exit code: " << status.errorCode << "\n";

	g_receiveOutputThread = thread(ReceiveOutput);
	if (g_receiveOutputThread.joinable())
		g_receiveOutputThread.join();

	if (!(status = injector.CallRemoteProc(g_modulePath, g_moduleName, "DeInit", NULL, 0)).success)
		cerr << "DeInit() failed: " << status.errorString << " " << status.errorCode << "\n";
	
	injector.UnloadModule(g_moduleName);
	injector.Detach();
	return 0;
}

void ReceiveOutput()
{
	BYTE buffer[BUFFER_SIZE] = { 0 };
	DWORD nBytesReceived = 0;
	HANDLE hPipe = NULL;
	stringstream ss;

	hPipe = CreateFileA(
		PIPE_SERVER_NAME,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (!hPipe){
		cerr << "Unable to open pipe to receive output: " << GetLastError() << "\n";
		return;
	}

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);

	while (g_appRunning){
		do{
			ReadFile(hPipe, buffer, sizeof(buffer), &nBytesReceived, NULL);
			if (nBytesReceived > 0){
				ss << buffer;
				RtlSecureZeroMemory(buffer, sizeof(buffer));
			}
		} while (GetLastError() == ERROR_MORE_DATA);
		cout << ss.str();
		ss.str(string());
	}
}