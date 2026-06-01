/// file      : parsers/macho/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Private snake_case backends for the Parsers/MachO namespace. Not part
/// of the documented public surface; the PascalCase macros in
/// <Misra/Parsers/MachO.h> are the only intended call shape.

#ifndef MISRA_PARSERS_MACHO_PRIVATE_H
#define MISRA_PARSERS_MACHO_PRIVATE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Macho        Macho;
typedef struct MachoSection MachoSection;

bool macho_open(Macho *out, Zstr path, Allocator *alloc);
bool macho_open_from_memory_copy(Macho *out, const u8 *data, size data_size, Allocator *alloc);
const MachoSection *macho_find_section(const Macho *self, Zstr segment, Zstr section);

#ifdef __cplusplus
}
#endif

#endif // MISRA_PARSERS_MACHO_PRIVATE_H
