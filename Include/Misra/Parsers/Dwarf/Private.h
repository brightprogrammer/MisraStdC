/// file      : parsers/dwarf/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Private snake_case backends for the Parsers/Dwarf namespace. Not part
/// of the documented public surface; the PascalCase macros in
/// <Misra/Parsers/Dwarf.h> are the only intended call shape.

#ifndef MISRA_PARSERS_DWARF_PRIVATE_H
#define MISRA_PARSERS_DWARF_PRIVATE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Elf            Elf;
typedef struct DwarfLines     DwarfLines;
typedef struct DwarfCfi       DwarfCfi;
typedef struct DwarfFunctions DwarfFunctions;

bool dwarf_lines_build_from_elf(DwarfLines *out, const Elf *elf, Allocator *alloc);
bool dwarf_cfi_build_from_elf(DwarfCfi *out, const Elf *elf, Allocator *alloc);
bool dwarf_functions_build_from_elf(DwarfFunctions *out, const Elf *elf, Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_PARSERS_DWARF_PRIVATE_H
