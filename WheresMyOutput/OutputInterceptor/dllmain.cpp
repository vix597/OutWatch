#include <Windows.h>
#include <iostream>

#include "named_pipe_server.h"
#include "mem_util.h"

using namespace std;

extern "C" {
	__declspec(dllexport) HRESULT Init(PVOID unused);
	__declspec(dllexport) HRESULT DeInit(PVOID unused);
}

/*
On 64-bit we have to store the jump address in RAX because there's
no push immediate for a 64-bit value

x86-64:
mov rax, <addr of hook>
push rax
ret
x86-32:
push <addr of hook>
ret
*/
#if defined(_WIN64)
static uint8_t hookBytes[] = { 0 };
static uint64_t jmpAddr = (uint64_t*)&hookBytes[2];//TODO: Fix this to be right
#else
static uint8_t hookBytes[] = { 0xFF, 0x35, 0x00, 0x00, 0x00, 0x00, 0xC3 };
static uint32_t * jmpAddr = (uint32_t*)&hookBytes[2];
#endif

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

	uint32_t fnwrite = 0x01470550;

	//
	//Set jump address in hook for printf
	//
	(*jmpAddr) = (uint32_t)write_hook;
	if (!SetVirtualMemory((LPVOID)fnwrite, 7, hookBytes)){
		cerr << "Cannot hook printf\n";
		return 1;
	}

	return S_OK;
}

HRESULT DeInit(PVOID unused)
{
	if (outputServer)
		delete outputServer;

	return S_OK;
}

void write_hook(int a, void * b, unsigned int c)
{
	FILE * f = fopen("C:\\AWESOME.txt", "w");
	char * out = "Output\n";
	fwrite(out, 1, sizeof(out), f);
	fclose(f);
	outputServer->SendData(c,(BYTE*)b);
}