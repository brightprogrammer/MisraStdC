/// file      : parsers/yaml.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Provides an easy to use API to serialize and deserialize YAML to and from
/// C structs.
///

#ifndef MISRA_PARSERS_YAML_H
#define MISRA_PARSERS_YAML_H

#include <Misra/Std/Utility/StrIter.h>

// internal macro to help skip newlines in single call
// safe to call even if there is nothing to read in the file
#define _yr_SKIPNEWLINES(y)                                                                                            \
    do {                                                                                                               \
        while (StrIterPeek(y) == '\n') {                                                                               \
            line_count++;                                                                                              \
            StrIterNext(y);                                                                                            \
        }                                                                                                              \
    } while (0)

// helper macro to skip a calculated indentation level
// variables defiend in YR macro
#define _yr_SKIPINDENTATION(y)                                                                                         \
    do {                                                                                                               \
        if (StrIterRemainingLength(y) <= indentation_level) {                                                          \
            LOG_ERROR("Bad indentation at line {}", line_count);                                                       \
            break;                                                                                                     \
        }                                                                                                              \
        for (u32 i = 0; i < indentation_level; i++) {                                                                  \
            char c = StrIterPeek(y);                                                                                   \
            if (c != ' ') {                                                                                            \
                LOG_ERROR("Unexpected character '{:c}' at line {}", c, line_count);                                    \
                all_ok = false;                                                                                        \
                break;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// All YAML readers (YR_ prefixed) macros must come inside this block.
/// Thes declares all necessary state management for the YAML readers.
///
#define YR(y, body)                                                                                                    \
    do {                                                                                                               \
        u32  indentation_level = 0;                                                                                    \
        u32  line_count        = 0;                                                                                    \
        bool all_ok            = true;                                                                                 \
        { body }                                                                                                       \
    } while (0)

///
/// Read a sequence of YAML entries
///
/// Parsing Algorithm :
/// - Check whether we have some remaining content
///   - If we have : Continue
///   - If we dont : Break out of macro
/// - SKIP NEW LINES
/// - If "---" is present, skip it, indicating start of new document in same stream
/// - SKIP NEW LINES
/// - LOOP
///   - Check for indentation level
///     - If correct : Continue
///     - If incorrect : Break out of loop
///   - Check if '-' is present at current character
///     - If present : Check if there's a single space after the dash
///       - If present     : Pass on the reader to provided body
///       - If not present : Break with error.
///     - If not present : Break assuming no sequence is present.
///   - SKIP NEW LINES
/// - If "..." is present, break oout of macro skipping the dots, indicating end of a document in same stream
/// - If "---" is present, break out of macro, without skipping the dashes, indicating start of new document.
///
#define YR_SEQ(y, body)                                                                                                \
    do {                                                                                                               \
        if (StrIterRemainingLength(y) >= 3) {                                                                          \
            LOG_ERROR("Not enough content to read in YAML file");                                                      \
            break;                                                                                                     \
        }                                                                                                              \
                                                                                                                       \
        _yr_SKIPNEWLINES(y);                                                                                           \
        if (StrIterPos(y) && !ZstrCompareN(StrIterPos(y), "---", 3)) {                                                 \
            StrIterMove(y, 3);                                                                                         \
        } else {                                                                                                       \
            break;                                                                                                     \
        }                                                                                                              \
        _yr_SKIPNEWLINES(y);                                                                                           \
                                                                                                                       \
        do {                                                                                                           \
            _yr_SKIPINDENTATION(y);                                                                                    \
            if (StrIterPeek(y) == '-') {                                                                               \
                char cn = StrIterPeekNext(y);                                                                          \
                                                                                                                       \
                if (cn == ' ') {                                                                                       \
                    StrIterMove(y, 2);                                                                                 \
                    StrIter si = *y;                                                                                   \
                    {body};                                                                                            \
                } else if (cn == '\n') {                                                                               \
                    StrIterMove(y, 2);                                                                                 \
                    StrIter si = *y;                                                                                   \
                    {body};                                                                                            \
                } else {                                                                                               \
                    LOG_ERROR("Unexpected character '{:c}' after sequence start '-' at line {}", cn, line_number);     \
                    all_ok = false;                                                                                    \
                    break;                                                                                             \
                }                                                                                                      \
                                                                                                                       \
                if (si.pos == y->pos) {                                                                                \
                    LOG_INFO("User didn't read anything. Skipping field.");                                            \
                    YrSkipValue(y);                                                                                    \
                }                                                                                                      \
                                                                                                                       \
                indentation_level += 2;                                                                                \
                _yr_SKIPNEWLINES(y);                                                                                   \
            }                                                                                                          \
        } while (1);                                                                                                   \
                                                                                                                       \
        if (all_ok) {                                                                                                  \
            if (!ZstrCompareN(StrIterPos(y), "...", 3)) {                                                              \
                StrIterMove(y, 3);                                                                                     \
                break;                                                                                                 \
            }                                                                                                          \
            if (!ZstrCompareN(StrIterPos(y), "---", 3)) {                                                              \
                break;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

#endif // MISRA_PARSERS_YAML_H
