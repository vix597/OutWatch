#pragma once

#include <stdint.h>
#include <Windows.h>
#include <iostream>
#include <string>

#if defined(_WIN64)
static uint8_t hookBytes[] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3, 0x58 };
static uint64_t * heapBufAddr = (uint64_t*)&hookBytes[2];

static uint8_t trampoline_static[] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3 };
static uint64_t * targetRet = (uint64_t*)&trampoline_static[2];
static uint64_t * hookAddr = (uint64_t*)&trampoline_static[13];
#else
static uint8_t hookBytes[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0x50, 0xE9, 0x00, 0x00, 0x00, 0x00, 0x58 };
static uint32_t * heapBufAddr = (uint32_t*)&hookBytes[1];
static uint32_t * hookOffset = (uint32_t*)&hookBytes[7];

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
	std::string errorString;
public:
	Hook();
	~Hook();
	bool InstallHook(LPVOID target, LPVOID hook);
	bool RemoveHook();
	std::string GetErrorString() const{
		return errorString;
	}
};