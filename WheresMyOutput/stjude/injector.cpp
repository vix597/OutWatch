#include <Shlwapi.h>

#include "injector.h"
#include "status.h"
#include "process_util.h"

using namespace std;

#pragma comment(lib,"Shlwapi.lib")

Injector::Injector() : 
	hProcess(NULL),
	processId(0)
{}

Injector::~Injector()
{
	if (hProcess)
		CloseHandle(hProcess);
}

Status Injector::Attach(const wstring& procName)
{
	int pid = 0;
	Status s = GetProcessIdByName(procName, &pid);
	if (!s.success)
		return s;
	return this->Attach(pid);
}

Status Injector::Attach(int pid)
{
	Status ret;
	if (hProcess){
		ret.errorString = "Cannot attach to pid: " + to_string(pid) + " already attached to process: " + to_string(processId);
		return ret;
	}

	ret = EnablePrivilege(SE_DEBUG_NAME, TRUE);
	if (!ret.success)
		return ret;
	ret.success = false;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProcess){
		ret.errorString = "Unable to open process: " + to_string(pid);
		ret.errorCode = GetLastError();
		return ret;
	}

	processId = pid;
	ret.success = true;
	return ret;
}

void Injector::Detach()
{
	if (hProcess)
		CloseHandle(hProcess);
	hProcess = NULL;
	processId = 0;
}

Status Injector::InjectModule(const wstring& modulePath)
{
	Status ret;
	if (!hProcess){
		ret.errorString = "Cannot inject module: " + string(modulePath.begin(), modulePath.end()) + " must be attached to a process first";
		return ret;
	}
	if (!PathFileExistsW(modulePath.c_str())){
		ret.errorString = "Cannot inject module: " + string(modulePath.begin(), modulePath.end()) + " path does not exit";
		return ret;
	}

	FARPROC fnLoadLibraryW = GetProcAddress(GetModuleHandleW(L"Kernel32"), "LoadLibraryW");
	if (!fnLoadLibraryW){
		ret.errorString = "Cannot get LoadLibraryW address";
		ret.errorCode = GetLastError();
		return ret;
	}

	return ExecRemoteProc(fnLoadLibraryW, (PVOID)modulePath.c_str(), (modulePath.length() * sizeof(wchar_t)) + sizeof(wchar_t));
}

Status Injector::UnloadModule(const wstring& moduleName)
{
	Status ret;
	if (!hProcess){
		ret.errorString = "Cannot unload: " + string(moduleName.begin(), moduleName.end()) + " must be attached to a process first.";
		return ret;
	}

	LPVOID moduleBaseAddress = NULL;
	ret = GetRemoteModuleHandle(processId, moduleName, &moduleBaseAddress);
	if (!ret.success)
		return ret;
	ret.success = false;

	FARPROC fnFreeLibrary = GetProcAddress(GetModuleHandleW(L"Kernel32"), "FreeLibrary");
	if (!fnFreeLibrary){
		ret.errorString = "Cannot get FreeLibrary address";
		ret.errorCode = GetLastError();
		return ret;
	}

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)fnFreeLibrary, (LPVOID)moduleBaseAddress, 0, NULL);
	if (!hThread){
		ret.errorString = "Unable to inject FreeLibrary() call into remote process";
		ret.errorCode = GetLastError();
		return ret;
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	ret.success = true;
	return ret;
}

Status Injector::CallRemoteProc(const wstring& modulePath, const wstring& moduleName, const string& functionName, PVOID arg, uint32_t argSize)
{
	Status ret;
	LPVOID remoteFuncAddress = NULL;
	ret = this->GetRemoteProcAddress(modulePath, moduleName, functionName, &remoteFuncAddress);
	if (!ret.success)
		return ret;
	return this->CallRemoteProc(remoteFuncAddress, arg, argSize);
}

Status Injector::CallRemoteProc(LPVOID remoteFuncAddr, PVOID arg, uint32_t argSize)
{
	return ExecRemoteProc(remoteFuncAddr, arg, argSize);
}

Status Injector::ExecRemoteProc(LPVOID function, PVOID arg, uint32_t argSize)
{
	LPVOID remoteArgAddress = NULL;
	SIZE_T nBytesWritten = 0;
	HANDLE hThread = NULL;
	Status ret;

	if (!hProcess){
		ret.errorString = "Cannot execute remote function, not attached to a process";
		return ret;
	}

	if (arg && argSize > 0){
		remoteArgAddress = VirtualAllocEx(hProcess, NULL, argSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!remoteArgAddress){
			ret.errorString = "Unable to allocate memory in the remote process";
			ret.errorCode = GetLastError();
			return ret;
		}

		BOOL sucess = WriteProcessMemory(hProcess, remoteArgAddress, arg, argSize, &nBytesWritten);
		if (!sucess || nBytesWritten != argSize){
			ret.errorString = "Unable to write in remote process memory";
			ret.errorCode = GetLastError();
			VirtualFreeEx(hProcess, remoteArgAddress, 0, MEM_RELEASE);
			return ret;
		}
	}

	hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)function, remoteArgAddress, 0, NULL);
	if (!hThread){
		ret.errorString = "Unable to inject the DLL into the remote process with CreateRemoteThread()";
		ret.errorCode = GetLastError();
	}
	else{
		WaitForSingleObject(hThread, INFINITE);
		ret.success = true;
	}

	if (remoteArgAddress)
		VirtualFreeEx(hProcess, remoteArgAddress, 0, MEM_RELEASE);
	if (hThread){
		DWORD exitCode = 0;
		GetExitCodeThread(hThread, &exitCode);
		CloseHandle(hThread);
		ret.errorCode = exitCode;
	}

	return ret;
}

Status Injector::GetRemoteProcAddress(const wstring& modulePath, const wstring& moduleName, const string& functionName, LPVOID * remoteProcAddr)
{
	Status ret;
	if (!hProcess){
		ret.errorString = "Cannot get proc address, must first be attached to a process";
		return ret;
	}

	LPVOID moduleBaseAddr = NULL;
	ret = GetRemoteModuleHandle(processId, moduleName, &moduleBaseAddr);
	if (!ret.success)
		return ret;
	ret.success = false;

	uint64_t offset = 0;
	ret = GetFunctionOffset(modulePath, functionName, &offset);
	if (!ret.success)
		return ret;
	ret.success = false;

	(*remoteProcAddr) = (LPVOID)((uint64_t)moduleBaseAddr + offset);
	ret.success = true;
	return ret;
}
