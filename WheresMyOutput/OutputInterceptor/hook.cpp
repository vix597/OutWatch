#include "hook.h"
#include "mem_util.h"

Hook::Hook() :
	hooked(false)
{}

Hook::~Hook()
{}

bool Hook::InstallHook(LPVOID target, LPVOID hook)
{
	if (hooked)
		return false;

	/*
	RtlCopyMemory(trampoline, target, sizeof(hookBytes));

	//
	//Set jump address in hook for printf
	//
	(*jmpAddr) = (uint32_t)write_hook;
	if (!SetVirtualMemory((LPVOID)fnwrite, sizeof(hookBytes), hookBytes)){
		cerr << "Cannot hook _write()\n";
		return 1;
	}
	*/
}

bool Hook::RemoveHook()
{
	if (!hooked)
		return false;
}

