#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>

#include "named_pipe_server.h"
#include "hook.h"

using namespace std;

#define STD_OUT (HANDLE)0x07
#define STD_ERR (HANDLE)0x0b

extern "C" {
	__declspec(dllexport) HRESULT Init(PVOID unused);
	__declspec(dllexport) HRESULT DeInit(PVOID unused);
	__declspec(dllexport) HRESULT InsertHook(PVOID unused);
}

static NamedPipeServer * outputServer = NULL;
static Hook hWriteFile;

void write_hook(
#ifndef _WIN64
	LPVOID reserved1,//The nature of our hook results in an extra 32-bit value on the stack
#endif
	HANDLE hFile, 
	LPCVOID lpBuffer, 
	DWORD nNumberOfBytesToWrite, 
	LPDWORD lpNumberOfBytesWritten, 
	LPOVERLAPPED lpOverlapped
);

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

HRESULT InsertHook(PVOID unused)
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

	if(!hWriteFile.InstallHook(fnWriteFile,write_hook)){
		msg << "HOOK ERROR: "<<hWriteFile.GetErrorString()<<"\n";
		outputServer->SendData(msg.str().length(), (BYTE*)msg.str().c_str());
		return 2;
	}

	return S_OK;
}

HRESULT DeInit(PVOID unused)
{
	if (outputServer)
		delete outputServer;

	return S_OK;
}

void write_hook(
#ifndef _WIN64
	LPVOID reserved1,
#endif
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped
)
{
	if (hFile != STD_OUT && hFile != STD_ERR){
		return;//SInce SendData() below calls WriteFile, we need this check to avoid
			   //a recursive hook loop
	}

	BYTE * bytes = new BYTE[nNumberOfBytesToWrite];
	RtlCopyMemory(bytes, lpBuffer, nNumberOfBytesToWrite);
	outputServer->SendData(nNumberOfBytesToWrite,bytes);
	delete[] bytes;

	return;
}