/*THANKS Decoda!!!

Decoda
Copyright (C) 2007-2013 Unknown Worlds Entertainment, Inc. 

This file is part of Decoda.

Decoda is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Decoda is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Decoda.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "hook_util.h"
#include <stdio.h>
#include <assert.h>

/**
 * Returns the size of the instruction at the specified point.
 */
int GetInstructionSize(void* address, unsigned char* opcodeOut, int* operandSizeOut) 
{ 
    
    // Modified from http://www.devmaster.net/forums/showthread.php?p=47381

    unsigned char* func = static_cast<unsigned char*>(address);

    if (opcodeOut != NULL)
    {
        *opcodeOut = 0;
    }

    if (operandSizeOut != NULL)
    {
        *operandSizeOut = 0;
    }

    if (*func != 0xCC) 
    { 
        // Skip prefixes F0h, F2h, F3h, 66h, 67h, D8h-DFh, 2Eh, 36h, 3Eh, 26h, 64h and 65h
        int operandSize = 4; 
        int FPU = 0; 
        while(*func == 0xF0 || 
              *func == 0xF2 || 
              *func == 0xF3 || 
             (*func & 0xFC) == 0x64 || 
             (*func & 0xF8) == 0xD8 ||
             (*func & 0x7E) == 0x62)
        { 
            if(*func == 0x66) 
            { 
                operandSize = 2; 
            }
            else if((*func & 0xF8) == 0xD8) 
            {
                FPU = *func++;
                break;
            }

            func++;
        }

        // Skip two-byte opcode byte 
        bool twoByte = false; 
        if(*func == 0x0F) 
        { 
            twoByte = true; 
            func++; 
        } 

        // Skip opcode byte 
        unsigned char opcode = *func++; 

        // Skip mod R/M byte 
        unsigned char modRM = 0xFF; 
        if(FPU) 
        { 
            if((opcode & 0xC0) != 0xC0) 
            { 
                modRM = opcode; 
            } 
        } 
        else if(!twoByte) 
        { 
            if((opcode & 0xC4) == 0x00 || 
               (opcode & 0xF4) == 0x60 && ((opcode & 0x0A) == 0x02 || (opcode & 0x09) == 0x9) || 
               (opcode & 0xF0) == 0x80 || 
               (opcode & 0xF8) == 0xC0 && (opcode & 0x0E) != 0x02 || 
               (opcode & 0xFC) == 0xD0 || 
               (opcode & 0xF6) == 0xF6) 
            { 
                modRM = *func++; 
            } 
        } 
        else 
        { 
            if((opcode & 0xF0) == 0x00 && (opcode & 0x0F) >= 0x04 && (opcode & 0x0D) != 0x0D || 
               (opcode & 0xF0) == 0x30 || 
               opcode == 0x77 || 
               (opcode & 0xF0) == 0x80 || 
               (opcode & 0xF0) == 0xA0 && (opcode & 0x07) <= 0x02 || 
               (opcode & 0xF8) == 0xC8) 
            { 
                // No mod R/M byte 
            } 
            else 
            { 
                modRM = *func++; 
            } 
        } 

        // Skip SIB
        if((modRM & 0x07) == 0x04 &&
           (modRM & 0xC0) != 0xC0)
        {
            func += 1;   // SIB
        }

        // Skip displacement
        if((modRM & 0xC5) == 0x05) func += 4;   // Dword displacement, no base 
        if((modRM & 0xC0) == 0x40) func += 1;   // Byte displacement 
        if((modRM & 0xC0) == 0x80) func += 4;   // Dword displacement 

        // Skip immediate 
        if(FPU) 
        { 
            // Can't have immediate operand 
        } 
        else if(!twoByte) 
        { 
            if((opcode & 0xC7) == 0x04 || 
               (opcode & 0xFE) == 0x6A ||   // PUSH/POP/IMUL 
               (opcode & 0xF0) == 0x70 ||   // Jcc 
               opcode == 0x80 || 
               opcode == 0x83 || 
               (opcode & 0xFD) == 0xA0 ||   // MOV 
               opcode == 0xA8 ||            // TEST 
               (opcode & 0xF8) == 0xB0 ||   // MOV
               (opcode & 0xFE) == 0xC0 ||   // RCL 
               opcode == 0xC6 ||            // MOV 
               opcode == 0xCD ||            // INT 
               (opcode & 0xFE) == 0xD4 ||   // AAD/AAM 
               (opcode & 0xF8) == 0xE0 ||   // LOOP/JCXZ 
               opcode == 0xEB || 
               opcode == 0xF6 && (modRM & 0x30) == 0x00)   // TEST 
            { 
                func += 1; 
            } 
            else if((opcode & 0xF7) == 0xC2) 
            { 
                func += 2;   // RET 
            } 
            else if((opcode & 0xFC) == 0x80 || 
                    (opcode & 0xC7) == 0x05 || 
                    (opcode & 0xF8) == 0xB8 ||
                    (opcode & 0xFE) == 0xE8 ||      // CALL/Jcc 
                    (opcode & 0xFE) == 0x68 || 
                    (opcode & 0xFC) == 0xA0 || 
                    (opcode & 0xEE) == 0xA8 || 
                    opcode == 0xC7 || 
                    opcode == 0xF7 && (modRM & 0x30) == 0x00) 
            { 
                func += operandSize; 
            } 
        } 
        else 
        {
            if(opcode == 0xBA ||            // BT 
               opcode == 0x0F ||            // 3DNow! 
               (opcode & 0xFC) == 0x70 ||   // PSLLW 
               (opcode & 0xF7) == 0xA4 ||   // SHLD 
               opcode == 0xC2 || 
               opcode == 0xC4 || 
               opcode == 0xC5 || 
               opcode == 0xC6) 
            { 
                func += 1; 
            } 
            else if((opcode & 0xF0) == 0x80) 
            {
                func += operandSize;   // Jcc -i
            }
        }

        if (opcodeOut != NULL)
        {
            *opcodeOut = opcode;
        }

        if (operandSizeOut != NULL)
        {
            *operandSizeOut = operandSize;
        }

    }

    return func - static_cast<unsigned char*>(address);

}

/**
 * Returns the number of bytes until the next break in instructions.
 */
int GetInstructionBoundary(void* function, int count)
{

    int boundary = 0;

    while (boundary < count)
    {
        unsigned char* func = static_cast<unsigned char*>(function) + boundary;
        boundary += GetInstructionSize(func);
    }

    return boundary;

}