/// file      : parsers/pdb/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Private snake_case backends for the Parsers/Pdb namespace. Not part of
/// the documented public surface; the PascalCase macros in
/// <Misra/Parsers/Pdb.h> are the only intended call shape.

#ifndef MISRA_PARSERS_PDB_PRIVATE_H
#define MISRA_PARSERS_PDB_PRIVATE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct Pdb Pdb;

    bool pdb_open(Pdb *out, Zstr path, Allocator *alloc);
    bool pdb_open_n(Pdb *out, Zstr path, size len, Allocator *alloc);
    bool pdb_open_from_memory_copy(Pdb *out, const u8 *data, size data_size, Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_PARSERS_PDB_PRIVATE_H
