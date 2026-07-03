/// file      : parsers/elf/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Private snake_case backends for the Parsers/Elf namespace. Not part of
/// the documented public surface; the PascalCase macros in
/// <Misra/Parsers/Elf.h> are the only intended call shape.

#ifndef MISRA_PARSERS_ELF_PRIVATE_H
#define MISRA_PARSERS_ELF_PRIVATE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct Elf        Elf;
    typedef struct ElfSection ElfSection;

    bool              elf_open(Elf *out, Zstr path, Allocator *alloc);
    bool              elf_open_n(Elf *out, Zstr path, size len, Allocator *alloc);
    bool              elf_open_from_memory_copy(Elf *out, const u8 *data, size data_size, Allocator *alloc);
    const ElfSection *elf_find_section_zstr(const Elf *self, Zstr name);
    const ElfSection *elf_find_section_str(const Elf *self, const Str *name);

#ifdef __cplusplus
}
#endif

#endif // MISRA_PARSERS_ELF_PRIVATE_H
