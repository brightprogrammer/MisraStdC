/// file      : Source/Misra/_StartWin.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Custom entry point for the Windows freestanding build. Replaces
/// `mainCRTStartup` (the default console-subsystem entry that drags
/// in UCRT initialisation, locale setup, exit handlers, exception
/// machinery, etc.) with a minimal `misra_start` that:
///
///   1. Calls kernel32!GetCommandLineA() to fetch the raw cmdline.
///   2. Tokenises it into argv[] (basic whitespace split + simple
///      double-quoted-arg support; not full Microsoft-shell semantics).
///   3. Calls user's main(argc, argv).
///   4. Calls kernel32!ExitProcess(rc).
///
/// Tell the linker about us via `/ENTRY:misra_start` in meson.
///
/// The file is compiled only on Windows when the libc-diet path is
/// enabled (the meson gate also requires clang-cl -- MSVC pulls
/// compiler-runtime helpers in ways we don't replace).

#include <Misra/Types.h>

#if PLATFORM_WINDOWS

// Forward-declare the kernel32 entry points we use. Avoids pulling
// <windows.h> here, which would drag in declarations for thousands
// of symbols and tempt callers into using UCRT-bound APIs.
#    define DECLSPEC __declspec(dllimport)
#    define WINAPI   __stdcall

typedef const char   *LPCSTR;
typedef unsigned long DWORD;

DECLSPEC LPCSTR WINAPI GetCommandLineA(void);
DECLSPEC void WINAPI   ExitProcess(DWORD uExitCode);

// Defined in _WinStubs.c (same per-target source set as this file).
// Seeds __security_cookie from BCryptGenRandom before any function
// with stack-canary instrumentation runs.
extern void __security_init_cookie(void);

// User's main. Implemented in Bin/<Tool>.c. The linker resolves at
// final-link time.
extern int main(int argc, char **argv);

// Buffers for parsed cmdline. Fixed sizes -- if any single tool is
// invoked with > 256 args or > 8 KiB of args, we silently truncate
// at the limits. Bin/ tools take a handful of args; not a real
// limit for the use cases that ship.
#    define MISRA_START_MAX_ARGS    256
#    define MISRA_START_CMDLINE_CAP 8192

static char  g_misra_start_cmdline[MISRA_START_CMDLINE_CAP];
static char *g_misra_start_argv[MISRA_START_MAX_ARGS];

// Tokenise a (mutable) command line string into argv. Rules:
//   - Whitespace (space, tab) separates args.
//   - Double-quoted spans are taken literally; the surrounding
//     quotes are stripped. No escape sequence inside.
//   - No backslash handling, no env var expansion, no globbing.
// Returns argc; argv entries are null-terminated pointers into the
// original buffer (which we mutate in place by writing nulls at
// separator positions).
static int parse_cmdline(char *cmd, char **argv, int max_args) {
    int   argc = 0;
    char *p    = cmd;
    while (*p) {
        // Skip leading whitespace.
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (!*p) {
            break;
        }
        if (argc >= max_args) {
            break;
        }
        if (*p == '"') {
            // Quoted: span until matching quote (or end of string).
            // Strip the quotes from the resulting argv entry.
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') {
                p++;
            }
            if (*p) {
                *p = 0;
                p++;
            }
        } else {
            // Bare token: span until next whitespace (or end).
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') {
                p++;
            }
            if (*p) {
                *p = 0;
                p++;
            }
        }
    }
    return argc;
}

// The custom entry point. `void` parameter, `void` return, but the
// last thing we do is ExitProcess so control never returns to the
// linker's epilogue.
//
// no_stack_protector on the entry itself: __security_cookie still
// holds its pre-init sentinel when this function's prologue reads
// it, so the canary slot would be set from the wrong value.
// Skipping instrumentation here means every other function in the
// program reads the post-init (BCryptGenRandom-seeded) cookie.
__attribute__((no_stack_protector)) void misra_start(void) {
    __security_init_cookie();

    LPCSTR raw = GetCommandLineA();

    // Copy into our mutable buffer. parse_cmdline writes nulls into
    // the buffer to split args, so we can't mutate the kernel-owned
    // GetCommandLineA buffer in place.
    int i = 0;
    while (raw[i] && i < (int)sizeof(g_misra_start_cmdline) - 1) {
        g_misra_start_cmdline[i] = raw[i];
        i++;
    }
    g_misra_start_cmdline[i] = 0;

    int argc = parse_cmdline(g_misra_start_cmdline, g_misra_start_argv, MISRA_START_MAX_ARGS);
    int rc   = main(argc, g_misra_start_argv);
    ExitProcess((DWORD)rc);
}

#endif // PLATFORM_WINDOWS
