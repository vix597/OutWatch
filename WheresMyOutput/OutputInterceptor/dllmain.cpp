#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>

#include "named_pipe_server.h"
#include "mem_util.h"

using namespace std;

#define STD_OUT (HANDLE)0x07
#define STD_ERR (HANDLE)0x0b

#if defined(_WIN64)
static uint8_t hookBytes[] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3};
static uint64_t * hookAddr = (uint64_t*)&hookBytes[2];
#else
static uint8_t hookBytes[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3};
static uint32_t * hookAddr = (uint32_t*)&hookBytes[1];
#endif

extern "C" {
	__declspec(dllexport) HRESULT Init(PVOID unused);
	__declspec(dllexport) HRESULT DeInit(PVOID unused);
	__declspec(dllexport) HRESULT Hook(PVOID unused);
}

static NamedPipeServer * outputServer = NULL;

void write_hook(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

BOOL APIENTRY DllMain(IN HINSTANCE hDll, IN DWORD reason, IN LPVOID reserved)
{
	return TRUE;
}

HRESULT Init(PVOID unused)
{	
	if (!outputServer)
		outputServer = new NamedPipeServer();	
	return S_OK;
}

HRESULT Hook(PVOID unused)
{
	std::stringstream msg;

	if (!outputServer)
		return 1;
	
	FARPROC fnWriteFile = GetProcAddress(GetModuleHandleA("Kernel32"), "WriteFile");
	if (!fnWriteFile){
		msg << "Cannot get address of WriteFile: " << GetLastError() << "\n";
		outputServer->SendData(msg.str().length(), (BYTE*)msg.str().c_str());
		return 1;
	}

	msg << "Found WriteFile: 0x" << std::hex << fnWriteFile << "\n";
	outputServer->SendData(msg.str().length(), (BYTE*)msg.str().c_str());

#if defined(_WIN64)
	(*hookAddr) = (uint64_t)write_hook;
#else
	(*hookAddr) = (uint32_t)write_hook;
#endif

	if (!SetVirtualMemory((LPVOID)fnWriteFile, sizeof(hookBytes), hookBytes)){
		msg << "Cannot set hook: " << GetLastError() << "\n";
		outputServer->SendData(msg.str().length(), (BYTE*)msg.str().c_str());
		return 2;
	}

	// Flush the cache so we know that our new code gets executed.
	FlushInstructionCache(GetCurrentProcess(), NULL, NULL);

	return S_OK;
}

HRESULT DeInit(PVOID unused)
{
	if (outputServer)
		delete outputServer;

	return S_OK;
}

void write_hook(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	if (hFile != STD_OUT && hFile != STD_ERR){
		return;
	}

	BYTE * bytes = new BYTE[nNumberOfBytesToWrite];
	RtlCopyMemory(bytes, lpBuffer, nNumberOfBytesToWrite);
	outputServer->SendData(nNumberOfBytesToWrite,bytes);
	delete[] bytes;

	(*lpNumberOfBytesWritten) = nNumberOfBytesToWrite;
	return;
}