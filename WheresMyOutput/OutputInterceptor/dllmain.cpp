#include <Windows.h>
#include <iostream>

#include "named_pipe_server.h"

using namespace std;

extern "C" {
	__declspec(dllexport) HRESULT Init(PVOID unused);
	__declspec(dllexport) HRESULT DeInit(PVOID unused);
}

static NamedPipeServer * outputServer = NULL;

void write_hook(int a, void * b, unsigned int c);

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

HRESULT DeInit(PVOID unused)
{
	if (outputServer)
		delete outputServer;

	return S_OK;
}

void write_hook(int fd, void * data, unsigned int size)
{
	BYTE * bytes = new BYTE[size];
	RtlCopyMemory(bytes, data, size);
	outputServer->SendData(size,(BYTE*)bytes);
}