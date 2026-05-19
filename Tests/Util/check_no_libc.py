#!/usr/bin/env python3
"""no-libc check for MisraStdC sources.

================================================================
WHY THIS TEST EXISTS
================================================================
MisraStdC is a libc REPLACEMENT, not a libc wrapper. Pulling in
standard C library headers (`stdio.h`, `stdlib.h`, `string.h`, etc.)
defeats the entire project's purpose: every binary that uses MisraStdC
then transitively pulls in libc, undoing the in-tree-everything design.

Direct OS calls are fine. POSIX (`sys/mman.h`, `unistd.h`, `pthread.h`,
`fcntl.h`, `signal.h`, ...) and Win32 (`windows.h`, `winsock2.h`, ...)
are operating-system interfaces, not libc. They are how MisraStdC
talks to the kernel without going through the C runtime.

What is NOT fine is anything in libc proper:

    libc header     ->  use this MisraStdC equivalent
    -------------------------------------------------------------
    stdio.h         ->  Misra/Std/Io.h  (WriteFmt, WriteFmtLn,
                        StrAppendFmt, StrWriteFmt, ...)
    stdlib.h malloc ->  HeapAllocator / ArenaAllocator /
                        SlabAllocator / BudgetAllocator
    stdlib.h strtol ->  Misra/Std/Zstr.h  (ZstrToI64)
    stdlib.h strtod ->  Misra/Std/Zstr.h  (ZstrToF64)
    string.h strlen ->  Misra/Std/Zstr.h  (ZstrLen)
    string.h strcmp ->  Misra/Std/Zstr.h  (ZstrCompare /
                        ZstrCompareN / ZstrCompareIgnoreCase)
    string.h memcpy ->  Misra/Std/Memory.h  (MemCopy)
    string.h memmove->  Misra/Std/Memory.h  (MemMove)
    string.h memset ->  Misra/Std/Memory.h  (MemSet)
    string.h memcmp ->  Misra/Std/Memory.h  (MemCompare)
    stdint.h uXX/iXX-> Misra/Types.h  (u8 u16 u32 u64 i8 i16 i32 i64
                        size). For `uintptr_t`, cast pointer through
                        `u64` -- every supported target is 64-bit.
    stddef.h size_t ->  Misra/Types.h  (size). NULL is also in Types.h.
    stdarg.h        ->  Misra/Types.h re-exports va_list / va_start /
                        va_end where needed (printf-style FMT macros).
    errno.h         ->  Misra/Std/Log.h  (LOG_SYS_ERROR captures errno
                        and formats it without exposing the symbol).
    math.h          ->  Misra/Std/Container/Float.h  (FloatMath has
                        in-tree replacements for the common ops; we
                        don't link libm).
    ctype.h         ->  Write the predicate inline. A four-character
                        check for whitespace doesn't need a libc table.
    assert.h        ->  Misra/Std/Log.h  (LOG_FATAL; deadend tests
                        catch it via setjmp inside TestRunner.c).
    limits.h        ->  Misra/Types.h has typed limits (I32_MAX,
                        U64_MAX, ...).
    float.h         ->  Misra/Std/Container/Float.h.
    stdbool.h       ->  Misra/Types.h already provides bool/true/false.
    inttypes.h      ->  Misra/Types.h fixed-width types.
    setjmp.h        ->  Reserved exclusively for the deadend test
                        driver (TestRunner.c). The library proper has
                        no recovery from LOG_FATAL by design.
    time.h          ->  Misra/Sys.h timing helpers (when we add them).
    wchar.h, locale.h, memory.h, strings.h
                    ->  Project does not support these. If you need
                        wide / multibyte / locale: file a separate
                        design discussion before adding any of them.

================================================================
WHEN THIS TEST FAILS
================================================================
The output below lists every libc include site, grouped by header,
with the recommended replacement. Fix the source -- do not extend the
EXEMPTIONS list unless there is genuinely no MisraStdC alternative
AND the file lives at the OS boundary. Every entry in EXEMPTIONS is
documented debt; keep the list short and shrinking.

If this test surprises you on a clean checkout, that means the
project state has libc debt not yet paid down. The cleanup is a
sequence of commits, not a single sweep -- pick a category from
the report below (e.g. "all stdio.h sites") and convert one class at
a time.
"""

import os
import re
import sys

# The libc headers we will not accept anywhere except in EXEMPTIONS.
LIBC_HEADERS = {
    "stdio.h",
    "stdlib.h",
    "string.h",
    "stdint.h",
    "stddef.h",
    "stdbool.h",
    "errno.h",
    "limits.h",
    "ctype.h",
    "assert.h",
    "math.h",
    "time.h",
    "inttypes.h",
    "setjmp.h",
    "stdarg.h",
    "wchar.h",
    "locale.h",
    "memory.h",
    "strings.h",
    "float.h",
}

# Suggested replacement per banned header. Shown in the failure
# report. Keep these in sync with the docblock above.
REPLACEMENTS = {
    "stdio.h":    "Misra/Std/Io.h (WriteFmt, WriteFmtLn, StrAppendFmt, StrWriteFmt)",
    "stdlib.h":   "Misra/Std/Allocator (Heap/Slab/Arena/Budget) + Misra/Std/Zstr.h (ZstrToI64, ZstrToF64)",
    "string.h":   "Misra/Std/Memory.h (MemCopy, MemMove, MemSet, MemCompare) + Misra/Std/Zstr.h (ZstrLen, ZstrCompare*)",
    "stdint.h":   "Misra/Types.h (u8..u64, i8..i64, size). For uintptr_t, cast through u64.",
    "stddef.h":   "Misra/Types.h (size, NULL)",
    "stdbool.h":  "Misra/Types.h (bool, true, false)",
    "errno.h":    "Misra/Std/Log.h (LOG_SYS_ERROR captures errno without exposing the symbol)",
    "limits.h":   "Misra/Types.h typed limits (I32_MAX, U64_MAX, ...)",
    "ctype.h":    "Write the predicate inline (`c == ' '`, etc.) -- no libc table lookup",
    "assert.h":   "Misra/Std/Log.h (LOG_FATAL / LOG_ASSERT)",
    "math.h":     "Misra/Std/Container/Float.h (in-tree FloatMath); libm is not linked",
    "time.h":     "Misra/Sys.h timing helpers (file an issue if missing)",
    "inttypes.h": "Misra/Types.h fixed-width types",
    "setjmp.h":   "MisraStdC has no abort-recovery; reserved for Tests/Util/TestRunner.c only",
    "stdarg.h":   "Misra/Types.h re-exports va_list / va_start / va_end",
    "wchar.h":    "Not supported; open a design discussion before adding wide-char support",
    "locale.h":   "Not supported",
    "memory.h":   "Misra/Std/Memory.h",
    "strings.h":  "Misra/Std/Zstr.h (ZstrCompareIgnoreCase / ZstrCompareNIgnoreCase)",
    "float.h":    "Misra/Std/Container/Float.h or Misra/Types.h",
}

# Pinned exemptions. Each entry: (relative-path, header, reason).
# Every entry is documented debt. Keep this list short. Do not
# extend it to mask new violations -- fix the source instead.
EXEMPTIONS = {
    # The deadend test driver uses setjmp/longjmp to capture LOG_FATAL
    # aborts so we can assert that bad inputs trigger fatals. The
    # library proper has no recovery mechanism (LOG_FATAL is
    # contractually unrecoverable), so only the test harness needs
    # this header.
    ("Tests/Util/TestRunner.c", "setjmp.h"): "deadend test driver captures LOG_FATAL via longjmp",
}

# Directories to scan, relative to repo root.
SCAN_DIRS = ("Source", "Include", "Bin", "Tests")

# File suffixes we consider.
SUFFIXES = (".c", ".h", ".cpp", ".hpp", ".inc")

# Match `#include <header>` with optional trailing comment / whitespace.
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*<([^>]+)>')


def find_repo_root() -> str:
    """Walk up until we hit a directory that owns the .git tree -- that
    is the project root, regardless of how many sub-`meson.build`
    files sit between us and it."""
    here = os.path.dirname(os.path.abspath(__file__))
    cur = here
    while cur != os.path.dirname(cur):
        if os.path.isdir(os.path.join(cur, ".git")):
            return cur
        cur = os.path.dirname(cur)
    return here


def scan_file(path: str, root: str):
    """Return list of (relpath, lineno, header) for libc includes."""
    rel = os.path.relpath(path, root)
    findings = []
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as fh:
            for lineno, line in enumerate(fh, start=1):
                m = INCLUDE_RE.match(line)
                if not m:
                    continue
                hdr = m.group(1).strip()
                # Only match exact libc header filename (allow leading
                # path-free form). A POSIX include like sys/mman.h is
                # never in LIBC_HEADERS because it contains a slash.
                if hdr in LIBC_HEADERS:
                    findings.append((rel, lineno, hdr))
    except OSError:
        pass
    return findings


def main() -> int:
    root = find_repo_root()
    all_findings = []
    for sub in SCAN_DIRS:
        base = os.path.join(root, sub)
        if not os.path.isdir(base):
            continue
        for dirpath, dirnames, filenames in os.walk(base):
            # Skip build directories and hidden trees.
            dirnames[:] = [d for d in dirnames if not d.startswith(".") and d != "build"]
            for fn in filenames:
                if not fn.endswith(SUFFIXES):
                    continue
                all_findings.extend(scan_file(os.path.join(dirpath, fn), root))

    # Apply exemptions.
    violations = []
    for rel, lineno, hdr in all_findings:
        if (rel, hdr) in EXEMPTIONS:
            continue
        violations.append((rel, lineno, hdr))

    if not violations:
        print("[no-libc] clean: zero libc includes in scanned tree")
        return 0

    # Group by header for a focused report.
    print("=" * 72)
    print("MisraStdC: libc includes detected -- this test ENFORCES")
    print("the project's in-tree-everything rule. See the docblock at")
    print("the top of Tests/Util/check_no_libc.py for the full why.")
    print("=" * 72)

    by_hdr = {}
    for rel, lineno, hdr in violations:
        by_hdr.setdefault(hdr, []).append((rel, lineno))

    for hdr in sorted(by_hdr):
        sites = by_hdr[hdr]
        rep = REPLACEMENTS.get(hdr, "(no MisraStdC equivalent registered)")
        print()
        print(f"<{hdr}>  ({len(sites)} site(s))")
        print(f"  --> {rep}")
        for rel, lineno in sorted(sites):
            print(f"     {rel}:{lineno}")

    print()
    print("-" * 72)
    print(f"TOTAL: {len(violations)} libc include site(s) across {len(by_hdr)} header(s)")
    print()
    print("Direct OS interfaces (sys/*.h, unistd.h, pthread.h, signal.h,")
    print("fcntl.h, windows.h, etc.) are fine -- they are the kernel")
    print("boundary, not libc. The bans above are about the C standard")
    print("library proper.")
    print()
    print("If a site is genuinely irreplaceable, add a (path, header)")
    print("entry to EXEMPTIONS at the top of this script WITH a one-line")
    print("rationale. Do not extend the list to whitewash debt.")
    print("-" * 72)
    return 1


if __name__ == "__main__":
    sys.exit(main())
