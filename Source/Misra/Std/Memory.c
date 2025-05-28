/// file      : std/memory.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
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
