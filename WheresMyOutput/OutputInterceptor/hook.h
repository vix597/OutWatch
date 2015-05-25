#pragma once

#include <stdint.h>
#include <Windows.h>

/*
Hook bytes

x86-64 hook:

movabs rax, <addr of buffer>
push rax
ret
pop rax

trampoline static x86-64:

push rax
movabs rax, <addr of "pop rax" in hook>
push rax
movabs rax, <addr of hook>
ret

x86-32 hook:

mov eax, <addr of buffer>
push eax
ret
pop eax

trampoine static x86-32:

push eax
mov eax, <addr of "pop eax" in hook>
push eax
mov eax, <addr of hook>
ret
*/
#if defined(_WIN64)
static uint8_t hookBytes[] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3, 0x58 };
static uint64_t * heapBufAddr = (uint64_t*)&hookBytes[2];

static uint8_t trampoline_static[] = { 0x50, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3 };
static uint64_t * targetRet = (uint64_t*)&trampoline_static[3];
static uint64_t * targetRet = (uint64_t*)&trampoline_static[14];
#else
static uint8_t hookBytes[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3, 0x58 };
static uint32_t * heapBufAddr = (uint32_t*)&hookBytes[8];
static uint32_t * hookAddr = (uint32_t*)&hookBytes[1];

static uint8_t trampoline_static[] = { 0x50, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3 };
static uint32_t * targetRet = (uint32_t*)&trampoline_static[2];
#endif

class Hook{
private:
	LPVOID hook_target;
	HANDLE trampoline_heap;
	uint8_t * trampoline;
	uint8_t * preHookBytes;
	int boundary;
	bool hooked;
public:
	Hook();
	~Hook();
	bool InstallHook(LPVOID target, LPVOID hook);
	bool RemoveHook();
};