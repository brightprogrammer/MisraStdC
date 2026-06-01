/// file      : sys/_helpers.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Path-string helpers shared between the Sys backtrace / symbol-resolver
/// stack and the OS-specific cache modules (MachoCache, PdbCache,
/// SymbolResolver, Backtrace). Underscore-prefixed so the build does not
/// install it under Include/Misra/.

#ifndef MISRA_SYS_HELPERS_H
#define MISRA_SYS_HELPERS_H

#include <Misra/Std/File.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

/// Truthy iff `path` opens for read. Implementation detail: this
/// momentarily opens and closes the file; on Windows the close races a
/// concurrent unlink, which is acceptable for the existence-probe
/// callers (cache lookups, sidecar discovery).
static inline bool sys_path_exists(Zstr path) {
    File f = FileOpen(path, "rb");
    if (!FileIsOpen(&f)) {
        return false;
    }
    FileClose(&f);
    return true;
}

/// Pointer to the last path component in `path` (after the final
/// '/' or '\\'). Returns "" on NULL; the returned pointer aliases
/// `path` itself, so it stays valid for `path`'s lifetime.
static inline Zstr sys_basename_of(Zstr path) {
    if (!path)
        return "";
    Zstr base = path;
    for (Zstr p = path; *p; ++p) {
        if (*p == '/' || *p == '\\')
            base = p + 1;
    }
    return base;
}

/// Append the directory prefix of `path` (everything up to but not
/// including the last separator) into `out`. No-op on NULL or on paths
/// with no separator.
static inline void sys_append_dirname(Str *out, Zstr path) {
    if (!path)
        return;
    Zstr last_sep = NULL;
    for (Zstr p = path; *p; ++p) {
        if (*p == '/' || *p == '\\')
            last_sep = p;
    }
    if (!last_sep)
        return;
    u64 len = (u64)(last_sep - path);
    for (u64 i = 0; i < len; ++i)
        StrPushBackR(out, path[i]);
}

#endif // MISRA_SYS_HELPERS_H
