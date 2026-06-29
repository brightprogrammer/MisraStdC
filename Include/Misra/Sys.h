/// file      : sys.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable system functions. Umbrella header: a pure include manifest that
/// re-exports the foundation primitives (`Sys/Foundation.h`) and the enabled
/// optional `Sys/*` features. The declarations themselves live in their own
/// headers -- nothing is declared here directly.

#ifndef MISRA_SYS_H
#define MISRA_SYS_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Clock.h>
#include <Misra/Sys/Errno.h>
#include <Misra/Sys/Foundation.h>
#include <Misra/Sys/Mutex.h>

#if FEATURE_SYS_DIR
#    include <Misra/Sys/Dir.h>
#endif

#if FEATURE_SYS_PROC
#    include <Misra/Sys/Proc.h>
#endif

#if FEATURE_SYS_SOCKET
#    include <Misra/Sys/Socket.h>
#endif

#if FEATURE_SYS_PROCMAPS
#    include <Misra/Parsers/ProcMaps.h>
#endif

#if FEATURE_SYS_DNS
#    include <Misra/Sys/Dns.h>
#endif

// Sys/SymbolResolver.h, Sys/Backtrace.h, Sys/PdbCache.h, and
// Sys/MachoCache.h are NOT pulled through the umbrella because they
// transit Parsers/Elf.h / Parsers/Pdb.h / Parsers/Macho.h, whose
// public-API names can collide with downstream TUs that carry their
// own ELF / PDB / Mach-O constants. Include those headers directly
// when you want the resolver, the backtrace formatter, or the
// PE+PDB / Mach-O symbol caches.

#endif // MISRA_SYS_H
