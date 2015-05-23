#pragma once

#include <Windows.h>
#include <iostream>
#include <stdint.h>

struct Status;

class Injector{
private:
	HANDLE hProcess;
	int processId;
private:
	Status ExecRemoteProc(LPVOID func, PVOID arg, uint32_t argSize);
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