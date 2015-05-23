#include "process_util.h"
#include "status.h"

#include <Windows.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include <iostream>

#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"advapi32.lib")

using namespace std;

//
//Compute the offset to a function within a module
//
Status GetFunctionOffset(const wstring& library, const string& functionName, uint64_t * funcOffset)
{
	Status ret;
	uint64_t offset = 0;
	HMODULE hLoaded = LoadLibraryW(library.c_str());
	if (!hLoaded){
		ret.errorString = "Failed to load library: " + string(library.begin(), library.end());
		ret.errorCode = GetLastError();
		return ret;
	}

	LPVOID functionAddress = GetProcAddress(hLoaded, functionName.c_str());
	if (!functionAddress){
		ret.errorString = "Unable to load procedure: " + functionName + " from library: " + string(library.begin(), library.end());
		ret.errorCode = GetLastError();
		return ret;
	}

	offset = (uint64_t)functionAddress - (uint64_t)hLoaded;

	FreeLibrary(hLoaded);
	(*funcOffset) = offset;
	ret.success = true;
	return ret;
}

Status GetProcessIdByName(const wstring& processName, int * processId)
{
	Status ret;
	PROCESSENTRY32W pe32;
	HANDLE hSnapshot = INVALID_HANDLE_VALUE;

	//Get snapshot of all processes
	pe32.dwSize = sizeof(PROCESSENTRY32W);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	//Can we start looking?
	if (!Process32FirstW(hSnapshot, &pe32))
	{
		ret.errorString = "Unable to search process list";
		CloseHandle(hSnapshot);
		return ret;
	}

	//Enumerate all processes till we find the one we are looking for or until every one of them is checked
	while (wcscmp(pe32.szExeFile, processName.c_str()) != 0 && Process32NextW(hSnapshot, &pe32));

	//Close the handle
	CloseHandle(hSnapshot);

	//Check if process id was found and return it
	if (wcscmp(pe32.szExeFile, processName.c_str()) == 0){
		ret.success = true;
		(*processId) = pe32.th32ProcessID;
		return ret;
	}

	ret.errorString = "Process: " + string(processName.begin(), processName.end()) + " not found";
	return ret;
}

Status GetRemoteModuleHandle(int processId, const wstring& moduleName, LPVOID * moduleBaseAddr)
{
	Status ret;
	MODULEENTRY32W me32;
	HANDLE hSnapshot = INVALID_HANDLE_VALUE;

	//Get snapshot of all modules in the remote process 
	me32.dwSize = sizeof(MODULEENTRY32W);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);

	//Can we start looking?
	if (!Module32FirstW(hSnapshot, &me32))
	{
		ret.errorString = "Unable to search module list in pid: " + to_string(processId);
		CloseHandle(hSnapshot);
		return ret;
	}

	//Enumerate all modules till we find the one we are looking for or until every one of them is checked
	while (wcscmp(me32.szModule, moduleName.c_str()) != 0 && Module32NextW(hSnapshot, &me32));

	//Close the handle
	CloseHandle(hSnapshot);

	//Check if module handle was found and return it
	if (wcscmp(me32.szModule, moduleName.c_str()) == 0){
		ret.success = true;
		(*moduleBaseAddr) = (LPVOID)me32.modBaseAddr;
		return ret;
	}

	ret.errorString = "Module: " + string(moduleName.begin(), moduleName.end()) + " not found in process: " + to_string(processId);
	return ret;
}

Status EnablePrivilege(const wstring& priviledgeName, bool enable)
{
	Status ret;

	//Get the access token of the current process
	HANDLE token;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)){
		ret.errorString = "Failed to open process token";
		ret.errorCode = GetLastError();
		return ret;
	}

	//Build the privilege
	TOKEN_PRIVILEGES privileges;
	RtlSecureZeroMemory(&privileges, sizeof(privileges));
	privileges.PrivilegeCount = 1;
	privileges.Privileges[0].Attributes = (enable) ? SE_PRIVILEGE_ENABLED : 0;
	if (!LookupPrivilegeValueW(NULL, priviledgeName.c_str(), &privileges.Privileges[0].Luid))
	{
		ret.errorString = "Unable to lookup privilege token value";
		ret.errorCode = GetLastError();
		CloseHandle(token);
		return ret;
	}

	// add the privilege
	BOOL result = AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), NULL, NULL);
	if (!result){
		ret.errorString = "Unable to adjust token priveledge";
		ret.errorCode = GetLastError();
		CloseHandle(token);
		return ret;
	}

	// close the handle
	CloseHandle(token);
	ret.success = true;
	return ret;
}