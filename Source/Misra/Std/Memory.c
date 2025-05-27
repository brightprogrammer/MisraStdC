/// file      : std/memory.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, All rights reserved.
///
/// Memory manipulation functions

#include <Misra/Types.h>

void* SetMemory(void* dest, int val, size n) {
    unsigned char* p = (unsigned char*)dest;
    while (n--) {
        *p++ = (unsigned char)val;
    }
    return dest;
} 
