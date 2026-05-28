/// file      : misra.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Single-include umbrella. Pulls in every Misra module that the current
/// build was configured with. Disabled features are skipped via the
/// `FEATURE_*` macros defined in the generated `Misra/Config.h`.

#ifndef MISRA_H
#define MISRA_H

#include <Misra/Types.h>
#include <Misra/Sys.h>
#include <Misra/Std.h>

#if FEATURE_PARSER_JSON
#    include <Misra/Parsers/JSON.h>
#endif

#if FEATURE_PARSER_KVCONFIG
#    include <Misra/Parsers/KvConfig.h>
#endif

#if FEATURE_PARSER_HTTP
#    include <Misra/Parsers/Http.h>
#endif

#if FEATURE_PARSER_DNS
#    include <Misra/Parsers/Dns.h>
#endif

// Note: the binary-format parsers
// (`Parsers/Elf.h`, `Parsers/Dwarf.h`, `Parsers/MachO.h`,
// `Parsers/Pe.h`, `Parsers/Pdb.h`) are intentionally NOT pulled
// through the umbrella. Downstream tools (and any TU) that maintain
// their own ELF / DWARF / Mach-O / PE / PDB constants would collide
// with the public-API names. Include the relevant header directly
// when you want the parser. Same carve-out applies to the
// SymbolResolver / Backtrace / PdbCache / MachoCache headers under
// `Misra/Sys/` -- see the note there.

#endif // MISRA_H
