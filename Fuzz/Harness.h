/// file      : fuzz/Harness.h
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Main harness header with common utilities

#ifndef FUZZ_HARNESS_H
#define FUZZ_HARNESS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Common data extraction functions
uint16_t extract_u16(const uint8_t *data, size_t *offset, size_t size);
uint8_t  extract_u8(const uint8_t *data, size_t *offset, size_t size);
uint32_t extract_u32(const uint8_t *data, size_t *offset, size_t size);

// Common string generation for char* vectors
char *generate_cstring(const uint8_t *data, size_t *offset, size_t size, size_t max_len);
void  cleanup_cstring(char *str);

// Note: Str generation function will be added later to avoid forward declaration conflicts

#endif // FUZZ_HARNESS_H
