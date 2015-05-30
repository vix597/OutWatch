#pragma once

#include <Windows.h>
#include <iostream>
#include <stdint.h>

typedef struct _CLIENT_ID
{
	PVOID UniqueProcess;
	PVOID UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef long(__stdcall *_RtlCreateUserThread)(
	HANDLE,					//ProcessHandle
	PSECURITY_DESCRIPTOR,	//SecurityDescriptor OPTIONAL
	BOOLEAN,				//CreateSuspended
	ULONG,					//StackZeroBits
	PULONG,					//StackReserver OUT
	PULONG,					//StackCommit OUT
	PVOID,					//StartAddress (PTHREAD_START_ROUTINE)
	PVOID,					//StartParameter OPTIONAL
	PHANDLE,				//ThreadHandle OUT
	PCLIENT_ID				//ClientId
);

struct Status;

class Injector{
private:
	HANDLE hProcess;
	int processId;
private:
	Status ExecRemoteProc(LPVOID func, PVOID arg, uint32_t argSize);
	Status ImpersonateUser();
	Status ForceCreateRemoteThread(PVOID func, PVOID remoteArgAddress);
public:
	Injector();
	~Injector();
public:
	Status Attach(const std::wstring& procName);
	Status Attach(int pid);
	void Detach();
	Status InjectModule(const std::wstring& modulePath);
	Status UnloadModule(const std::wstring& moduleName);
	Status CallRemoteProc(const std::wstring& modulePath, const std::wstring& moduleName, const std::string& functionName, PVOID arg, uint32_t argSize);
	Status CallRemoteProc(LPVOID remoteFuncAddr, PVOID arg, uint32_t argSize);
public:
	HANDLE GetProcessHandle() const { return hProcess; }
	Status GetRemoteProcAddress(const std::wstring& modulePath, const std::wstring& moduleName, const std::string& functionName, LPVOID * remoteProcAddr);
};