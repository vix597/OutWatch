#pragma once

#include <Windows.h>

int GetInstructionSize(void* address, unsigned char* opcodeOut = NULL, int* operandSizeOut = NULL);
int GetInstructionBoundary(void* function, int count);