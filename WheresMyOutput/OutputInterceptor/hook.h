#pragma once

#include <stdint.h>
#include <Windows.h>

/*
x86-64:
movabs rax, <addr of buffer>
push rax
ret
x86-32:
mov eax, <addr of buffer>
push eax
ret
ret
*/
#if defined(_WIN64)
static uint8_t hookBytes[] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3 };
static uint64_t jmpAddr = (uint64_t*)&hookBytes[2];
#else
static uint8_t hookBytes[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3 };
static uint32_t * jmpAddr = (uint32_t*)&hookBytes[1];
#endif

class Hook{
private:
	bool hooked;
public:
	Hook();
	~Hook();
	bool InstallHook(LPVOID target, LPVOID hook);
	bool RemoveHook();
};