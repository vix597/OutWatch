#include "hook.h"
#include "hook_util.h"
#include "mem_util.h"

Hook::Hook() :
	hook_target(NULL),
	trampoline_heap(NULL),
	trampoline(NULL),
	preHookBytes(NULL),
	boundary(0),
	hooked(false)
{}

Hook::~Hook()
{
	if (trampoline)
		HeapFree(trampoline_heap, 0, trampoline);
	if (trampoline_heap)
		HeapDestroy(trampoline_heap);
	if (preHookBytes)
		delete[] preHookBytes;
}

bool Hook::InstallHook(LPVOID target, LPVOID hook)
{
	if (hooked)
		return false;

	// Missing from windows.h (thanks https://github.com/unknownworlds/decoda)
	#define HEAP_CREATE_ENABLE_EXECUTE 0x00040000 

	boundary = GetInstructionBoundary(target, sizeof(hookBytes));

	//Create a heap to store trampoline functions
	trampoline_heap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 1024, 0);

	//Create trampoline
	trampoline = (uint8_t*)HeapAlloc(trampoline_heap,0,boundary + sizeof(trampoline_static));
	if (!trampoline){
		HeapDestroy(trampoline_heap);
		trampoline_heap = NULL;
		boundary = 0;
		return false;
	}

	preHookBytes = new uint8_t[boundary];
	RtlCopyMemory(preHookBytes, target, boundary);

#if defined(_WIN64)
	(*targetRet) = (uint64_t)target + (boundary - 1);//minus 1 for pop rax instruction
	(*hookAddr) = (uint64_t)hook;
	(*heapBufAddr) = (uint64_t)trampoline;
#else
	(*targetRet) = (uint32_t)target + (boundary - 1);//minus 1 for pop eax instruction
	(*hookAddr) = (uint32_t)hook;
	(*heapBufAddr) = (uint32_t)trampoline;
#endif

	RtlCopyMemory(trampoline, target, boundary);
	RtlCopyMemory(&trampoline[boundary], trampoline_static, sizeof(trampoline_static));

	DWORD protection = 0;
	if (VirtualProtect(target, boundary, PAGE_EXECUTE_READWRITE, &protection))
	{
		//Fill with nops
		memset(target, 0x90, boundary);
		
		//Write the hook
		RtlCopyMemory(target, hookBytes, sizeof(hookBytes));

		//Restore protections
		VirtualProtect(target, boundary, protection, &protection);

		// Flush the cache so we know that our new code gets executed.
		FlushInstructionCache(GetCurrentProcess(), NULL, NULL);

		hooked = true;
		hook_target = target;
		return true;
	}

	delete[] preHookBytes;
	preHookBytes = NULL;

	HeapFree(trampoline_heap, 0, trampoline);
	trampoline = NULL;

	HeapDestroy(trampoline_heap);
	trampoline_heap = NULL;

	boundary = 0;
	
	return false;
}

bool Hook::RemoveHook()
{
	if (!hooked || !hook_target)
		return false;

	DWORD protection = 0;
	if (VirtualProtect(hook_target, boundary, PAGE_EXECUTE_READWRITE, &protection))
	{
		RtlCopyMemory(hook_target, preHookBytes, boundary);
		VirtualProtect(hook_target, boundary, protection, &protection);
		FlushInstructionCache(GetCurrentProcess(), NULL, NULL);
		
		hooked = false;
		hook_target = NULL;
		boundary = 0;
		
		HeapFree(trampoline_heap, 0, trampoline);
		HeapDestroy(trampoline_heap);
		delete[] preHookBytes;

		preHookBytes = NULL;
		trampoline_heap = NULL;
		trampoline = NULL;
		return true;
	}

	return false;
}