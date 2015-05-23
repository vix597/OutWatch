#pragma once

#include <Windows.h>
#include <stdint.h>

bool SetVirtualMemory(LPVOID addr, uint64_t size, PVOID data);