#pragma once

#include <Windows.h>
#include <stdint.h>
#include <string>

struct Status;

Status GetFunctionOffset(const std::wstring& library, const std::string& functionName, uint64_t * funcOffset);

Status GetProcessIdByName(const std::wstring& processName, int * processId);

Status GetRemoteModuleHandle(int processId, const std::wstring& moduleName, LPVOID * moduleBaseAddr);

Status EnablePrivilege(const std::wstring& priviledgeName, bool enable);