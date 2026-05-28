// file      : Tests/Util/JsonReaderAllocAware.h
//
// JSON reader overrides used by the Tests/Json suites.
//
// `Parsers/JSON.h` builds keys / values via `StrInit()` (no allocator
// argument — uses the surrounding `MisraScope`). Tests sometimes need
// to hand the parser an explicit allocator so each test owns its
// allocations. These overrides take a caller-scope `alloc` symbol and
// thread it through `StrInit(&alloc)`.
//
// Cross-macro convention (same as JSON.h): `key` is intentionally NOT
// UNPL-wrapped — the inner `JR_*_KV` macros expanded by `reader` read
// it directly, so the name has to be reachable across macro boundaries.

#ifndef MISRA_TEST_JSON_READER_ALLOC_AWARE_H
#define MISRA_TEST_JSON_READER_ALLOC_AWARE_H

#include <Misra/Parsers/JSON.h>

#undef JR_OBJ
#define JR_OBJ(si, reader)                                                                                             \
    do {                                                                                                               \
        if (!StrIterRemainingLength(&si)) {                                                                            \
            break;                                                                                                     \
        }                                                                                                              \
        StrIter UNPL(saved_si) = si;                                                                                   \
        si                     = JSkipWhitespace(si);                                                                  \
        char UNPL(jr_c);                                                                                               \
        if (!StrIterPeek(&si, &UNPL(jr_c)) || UNPL(jr_c) != '{') {                                                     \
            LOG_ERROR("Invalid object start. Expected '{'.");                                                          \
            si = UNPL(saved_si);                                                                                       \
            break;                                                                                                     \
        }                                                                                                              \
        StrIterMustNext(&si);                                                                                          \
        si = JSkipWhitespace(si);                                                                                      \
        StrIter UNPL(read_si);                                                                                         \
        bool    UNPL(expect_comma) = false;                                                                            \
        bool    UNPL(failed)       = false;                                                                            \
        while (StrIterPeek(&si, &UNPL(jr_c)) && UNPL(jr_c) != '}') {                                                   \
            if (UNPL(expect_comma)) {                                                                                  \
                if (UNPL(jr_c) != ',') {                                                                               \
                    LOG_ERROR("Expected ',' after key/value pairs in object. Invalid JSON object.");                   \
                    UNPL(failed) = true;                                                                               \
                    si           = UNPL(saved_si);                                                                     \
                    break;                                                                                             \
                }                                                                                                      \
                StrIterMustNext(&si);                                                                                  \
                si = JSkipWhitespace(si);                                                                              \
            }                                                                                                          \
            Str key       = StrInit(&alloc);                                                                           \
            UNPL(read_si) = JReadString(si, &key);                                                                     \
            if (UNPL(read_si).pos == si.pos) {                                                                         \
                LOG_ERROR("Failed to read string key in object. Invalid JSON");                                        \
                StrDeinit(&key);                                                                                       \
                UNPL(failed) = true;                                                                                   \
                si           = UNPL(saved_si);                                                                         \
                break;                                                                                                 \
            }                                                                                                          \
            si = UNPL(read_si);                                                                                        \
            si = JSkipWhitespace(si);                                                                                  \
            if (!StrIterPeek(&si, &UNPL(jr_c)) || UNPL(jr_c) != ':') {                                                 \
                LOG_ERROR("Expected ':' after key string. Failed to read JSON");                                       \
                StrDeinit(&key);                                                                                       \
                UNPL(failed) = true;                                                                                   \
                si           = UNPL(saved_si);                                                                         \
                break;                                                                                                 \
            }                                                                                                          \
            StrIterMustNext(&si);                                                                                      \
            si                           = JSkipWhitespace(si);                                                        \
            StrIter UNPL(si_before_read) = si;                                                                         \
            { reader }                                                                                                 \
            if (UNPL(si_before_read).pos == si.pos) {                                                                  \
                StrIter UNPL(skip_si) = JSkipValue(si);                                                                \
                if (UNPL(skip_si).pos == si.pos) {                                                                     \
                    LOG_ERROR("Failed to parse value. Invalid JSON.");                                                 \
                    StrDeinit(&key);                                                                                   \
                    UNPL(failed) = true;                                                                               \
                    si           = UNPL(saved_si);                                                                     \
                    break;                                                                                             \
                }                                                                                                      \
                si = UNPL(skip_si);                                                                                    \
            }                                                                                                          \
            StrDeinit(&key);                                                                                           \
            si                 = JSkipWhitespace(si);                                                                  \
            UNPL(expect_comma) = true;                                                                                 \
        }                                                                                                              \
        if (!UNPL(failed)) {                                                                                           \
            if (!StrIterPeek(&si, &UNPL(jr_c)) || UNPL(jr_c) != '}') {                                                 \
                LOG_ERROR("Expected end of object '}' but found '{c}'", UNPL(jr_c));                                   \
                UNPL(failed) = true;                                                                                   \
                si           = UNPL(saved_si);                                                                         \
                break;                                                                                                 \
            }                                                                                                          \
            StrIterMustNext(&si);                                                                                      \
        }                                                                                                              \
    } while (0)

#undef JR_STR
#define JR_STR(si, str)                                                                                                \
    do {                                                                                                               \
        Str UNPL(my_str) = StrInit(&alloc);                                                                            \
        si               = JReadString((si), &UNPL(my_str));                                                           \
        (str)            = UNPL(my_str);                                                                               \
    } while (0)

#undef JR_STR_KV
#define JR_STR_KV(si, k, str)                                                                                          \
    do {                                                                                                               \
        if (!StrCmp(&key, (k))) {                                                                                  \
            Str UNPL(my_str) = StrInit(&alloc);                                                                        \
            si               = JReadString((si), &UNPL(my_str));                                                       \
            (str)            = UNPL(my_str);                                                                           \
        }                                                                                                              \
    } while (0)

#endif // MISRA_TEST_JSON_READER_ALLOC_AWARE_H
