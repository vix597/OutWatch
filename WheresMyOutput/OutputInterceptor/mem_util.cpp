#include "mem_util.h"

bool SetVirtualMemory(LPVOID addr, uint64_t size, void * data)
{
	DWORD oldProtect = 0;
	DWORD newProtext = PAGE_EXECUTE_READWRITE;

	if (!VirtualLock(addr, size)){
		return false;
	}

	if (!VirtualProtect(addr, size, newProtext, &oldProtect)){
		return false;
	}

	RtlSecureZeroMemory((PVOID)addr, size);
	RtlCopyMemory(addr, data, size);

	VirtualUnlock(addr, size);
	VirtualProtect(addr, size, oldProtect, &newProtext);
	
	return true;
}