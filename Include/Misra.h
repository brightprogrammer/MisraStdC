/// file      : misra.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Single-include umbrella. Pulls in every Misra module that the current
/// build was configured with. Disabled features are skipped via the
/// `MISRA_HAVE_*` macros defined in the generated `Misra/Config.h`.

#ifndef MISRA_H
#define MISRA_H

#include <Misra/Types.h>
#include <Misra/Sys.h>
#include <Misra/Std.h>

#if MISRA_HAVE_PARSER_JSON
#    include <Misra/Parsers/JSON.h>
#endif

#if MISRA_HAVE_PARSER_KVCONFIG
#    include <Misra/Parsers/KvConfig.h>
#endif

#if MISRA_HAVE_PARSER_HTTP
#    include <Misra/Parsers/Http.h>
#endif

#endif // MISRA_H
