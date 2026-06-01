/// file      : parsers/pe/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Private snake_case backends for the Parsers/Pe namespace. Not part of
/// the documented public surface; the PascalCase macros in
/// <Misra/Parsers/Pe.h> are the only intended call shape.

#ifndef MISRA_PARSERS_PE_PRIVATE_H
#define MISRA_PARSERS_PE_PRIVATE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Pe        Pe;
typedef struct PeSection PeSection;

bool pe_open(Pe *out, Zstr path, Allocator *alloc);
bool pe_open_from_memory_copy(Pe *out, const u8 *data, size data_size, Allocator *alloc);
const PeSection *pe_find_section_zstr(const Pe *self, Zstr name);
const PeSection *pe_find_section_str(const Pe *self, const Str *name);

#ifdef __cplusplus
}
#endif

#endif // MISRA_PARSERS_PE_PRIVATE_H
