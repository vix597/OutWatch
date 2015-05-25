#include <Windows.h>
#include <iostream>

#include "named_pipe_server.h"
#include "mem_util.h"

using namespace std;

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
}

static NamedPipeServer * outputServer = NULL;

BOOL write_hook(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

BOOL APIENTRY DllMain(IN HINSTANCE hDll, IN DWORD reason, IN LPVOID reserved)
{
	return TRUE;
}

HRESULT Init(PVOID unused)
{	
	if (!outputServer)
		outputServer = new NamedPipeServer();

	FARPROC fnWriteFile = GetProcAddress(GetModuleHandleA("Kernel32"), "WriteFile");

#if defined(_WIN64)
	(*hookAddr) = (uint64_t)write_hook;
#else
	(*hookAddr) = (uint32_t)write_hook;
#endif
	
	if (!SetVirtualMemory((LPVOID)fnWriteFile, sizeof(hookBytes), hookBytes))
		return 1;

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

BOOL write_hook(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	BYTE * bytes = new BYTE[nNumberOfBytesToWrite];
	RtlCopyMemory(bytes, lpBuffer, nNumberOfBytesToWrite);
	outputServer->SendData(nNumberOfBytesToWrite,(BYTE*)bytes);
	delete[] bytes;

	(*lpNumberOfBytesWritten) = nNumberOfBytesToWrite;
	return TRUE;
}