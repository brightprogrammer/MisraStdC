/// file      : parsers/kvconfig.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Key-value configuration reader. The parser walks a `StrIter` over
/// caller-owned config text and populates a `KvConfig` (typedef alias
/// for `Map(Str, Str)`) -- no parser object, no AST, no separate
/// `Init` / `Deinit` step beyond the map's own lifecycle. There is no
/// writer side: callers that need to render a config back out hand
/// the underlying `Map(Str, Str)` to their own formatter.
///
/// Line syntax is the common-denominator `.ini` / `.conf` shape: one
/// `key = value` (or `key: value`) per line, with `#` or `;` starting
/// a line comment and blank lines ignored. Values are trimmed when
/// unquoted; single- or double-quoted values preserve interior
/// whitespace and support backslash escapes. Duplicate keys are
/// resolved last-write-wins during `KvConfigParse`.
///
/// Typed extractors (`KvConfigGetBool` / `KvConfigGetI64` /
/// `KvConfigGetF64`) layer string->scalar parsing on top of the stored
/// `Str` so callers don't repeat the parse/validate dance per call
/// site. Accepted boolean spellings are `true`/`false`, `yes`/`no`,
/// `on`/`off`, `1`/`0`. Integer and float forms follow the
/// `ZstrToI64` / `ZstrToF64` rules (the entire trimmed value must
/// parse; any trailing junk is a failure, not a partial accept).
/// Each get-by-key entry point dispatches on `Str *` / `Zstr` /
/// `char *` via the standard `_Generic` shape.
///

#ifndef MISRA_PARSERS_KVCONFIG_H
#define MISRA_PARSERS_KVCONFIG_H

#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

// `KvConfig` is `Map(Str, Str)`. Because `Map(...)` expands to a fresh
// anonymous struct each time, the typedef must live in exactly one
// translation-unit-visible spot so the public header and the
// implementation agree on the layout.
typedef Map(Str, Str) KvConfig;

// Private backends. Included AFTER the KvConfig typedef so the
// snake_case prototypes there can name the type.
#include <Misra/Parsers/KvConfig/Private.h>

///
/// Key-value configuration map.
///
/// Duplicate keys are resolved with last-write-wins semantics during parsing.
///
/// USAGE:
///   // KvConfigInit + StrInitFromZstr resolve `MisraScope`; the body
///   // lives inside `Scope(...)`.
///   Scope(alloc, DefaultAllocator) {
///       KvConfig cfg = KvConfigInit();
///       Str text = StrInitFromZstr("host = localhost\nport = 8080\n", alloc);
///       StrIter input = StrIterFromStr(text);
///       StrIter si = KvConfigParse(input, &cfg);
///       (void)si;
///
///       Str *host_ptr = KvConfigGetPtr(&cfg, "host");
///       Str  host     = KvConfigGet(&cfg, "host");
///       i64  port = 0;
///       KvConfigGetI64(&cfg, "port", &port);
///       StrDeinit(&host);
///       StrDeinit(&text);
///       MapDeinit(&cfg);
///   }
///
/// FIELDS:
/// - inherited `Map(Str, Str)` fields from the underlying map storage.
///
/// TAGS: Parser, Config, KeyValue, Map
///

///
/// Initialize a `KvConfig` object with deep-copy ownership for both keys and values.
///
/// allocator_ptr : Allocator that owns the config's storage. May be a
///                 typed allocator handle (`&heap`) or a raw `Allocator *`.
///
/// SUCCESS : Returns initialized config object.
/// FAILURE : Macro cannot fail.
///
/// TAGS: KvConfig, Init, API
///
#define KvConfigInit(...) OVERLOAD(KvConfigInit, __VA_ARGS__)
#define KvConfigInit_0()  KvConfigInit_1(MisraScope)
#define KvConfigInit_1(allocator_ptr)                                                                                  \
    MapInitFull_9(                                                                                                     \
        str_hash,                                                                                                      \
        str_compare,                                                                                                   \
        str_compare,                                                                                                   \
        str_init_copy,                                                                                                 \
        str_deinit,                                                                                                    \
        str_init_copy,                                                                                                 \
        str_deinit,                                                                                                    \
        MapPolicyLinear,                                                                                               \
        (allocator_ptr)                                                                                                \
    )

// `KvConfig` is a thin typedef alias for `Map<Str, Str>`; use the Map
// API directly for lifecycle and inspection -- `MapDeinit(&cfg)`,
// `MapClear(&cfg)`, `MapPairCount(&cfg)`. The previous KvConfigDeinit /
// KvConfigClear / KvConfigLen macros were pure pass-throughs and have
// been removed (deadweight macro convention).

///
/// Skip horizontal whitespace in config text.
///
/// This skips `' '`, `'\t'`, and `'\r'`, but not `'\n'`.
///
/// si[in] : Iterator to advance.
///
/// SUCCESS : Returns updated iterator.
/// FAILURE : Function cannot fail. Returns `si` unchanged when already
///           at end of input or on a non-whitespace character.
///
/// TAGS: KvConfig, Skip, Whitespace
///
StrIter KvConfigSkipWhitespace(StrIter si);

///
/// Skip current line including trailing newline when present.
///
/// si[in] : Iterator positioned anywhere on a line.
///
/// SUCCESS : Returns iterator positioned at the first character of the next line.
/// FAILURE : Function cannot fail. Returns an iterator at end of input
///           when no trailing newline is present.
///
/// TAGS: KvConfig, Skip, Line
///
StrIter KvConfigSkipLine(StrIter si);

///
/// Read a config key from the current line.
///
/// Keys end before `=` / `:` / newline and are trimmed of surrounding whitespace.
///
/// si[in]   : Iterator at start of key.
/// key[out] : Parsed key.
///
/// SUCCESS : Returns iterator advanced to separator or following whitespace.
/// FAILURE : Returns original iterator on invalid key.
///
/// TAGS: KvConfig, Parse, Key, Read
///
StrIter KvConfigReadKey(StrIter si, Str *key);

///
/// Read a config value from the current line.
///
/// Unquoted values are trimmed. Quoted values preserve inner whitespace and support
/// basic backslash escapes.
///
/// si[in]     : Iterator at start of value.
/// value[out] : Parsed value.
///
/// SUCCESS : Returns iterator advanced to line end or end of input.
/// FAILURE : Returns original iterator on parse error.
///
/// TAGS: KvConfig, Parse, Value, Read
///
StrIter KvConfigReadValue(StrIter si, Str *value);

///
/// Read a single key-value pair.
///
/// Supported separators are `=` and `:`.
///
/// si[in]    : Iterator at start of a key-value line.
/// key[out]  : Parsed key.
/// value[out]: Parsed value.
///
/// SUCCESS : Returns iterator advanced past the parsed line.
/// FAILURE : Returns original iterator on parse error.
///
/// TAGS: KvConfig, Parse, Pair, Read
///
StrIter KvConfigReadPair(StrIter si, Str *key, Str *value);

///
/// Parse complete key-value config text into `cfg`.
///
/// Supported syntax:
/// - `key = value`
/// - `key: value`
/// - blank lines
/// - comments starting with `#` or `;`
/// - quoted values using `'` or `"`
///
/// If the same key appears multiple times, the last parsed value replaces earlier ones.
///
/// si[in]      : Input iterator.
/// cfg[in,out] : Destination config map.
///
/// SUCCESS : Returns iterator advanced to end of parsed config.
/// FAILURE : Returns original iterator on first invalid line.
///
/// TAGS: KvConfig, Parse, API
///
StrIter KvConfigParse(StrIter si, KvConfig *cfg);

///
/// Get stored value for `key` as a new `Str` copy.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Lookup key. Prefer `Str *`; `Zstr` accepted.
///
/// SUCCESS : Newly allocated copy of stored `Str` value. Caller must `StrDeinit(...)` it.
/// FAILURE : Empty `Str` if key does not exist.
///
/// TAGS: KvConfig, Get, API
///
#define KvConfigGet(cfg, key)                                                                                          \
    _Generic((key), Str *: kvconfig_get_str, Zstr: kvconfig_get_zstr, char *: kvconfig_get_zstr)((cfg), (key))

///
/// Get stored value for `key` by internal reference.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Lookup key. Prefer `Str *`; `Zstr` accepted.
///
/// SUCCESS : Pointer to stored `Str` value. Do not deinitialize or mutate through ownership-sensitive APIs.
/// FAILURE : `NULL` if key does not exist.
///
/// TAGS: KvConfig, Get, Pointer
///
#define KvConfigGetPtr(cfg, key)                                                                                       \
    _Generic((key), Str *: kvconfig_get_ptr_str, Zstr: kvconfig_get_ptr_zstr, char *: kvconfig_get_ptr_zstr)(          \
        (cfg),                                                                                                         \
        (key)                                                                                                          \
    )

///
/// Check whether a key exists in config.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Lookup key. Prefer `Str *`; `Zstr` accepted.
///
/// SUCCESS : `true` if key exists.
/// FAILURE : `false`
///
/// TAGS: KvConfig, Contains, API
///
#define KvConfigContains(cfg, key)                                                                                     \
    _Generic((key), Str *: kvconfig_contains_str, Zstr: kvconfig_contains_zstr, char *: kvconfig_contains_zstr)(       \
        (cfg),                                                                                                         \
        (key)                                                                                                          \
    )

///
/// Parse and fetch a boolean config value.
///
/// Accepted values: `true`, `false`, `yes`, `no`, `on`, `off`, `1`, `0`.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Lookup key. Prefer `Str *`; `Zstr` accepted.
/// value[out]  : Parsed boolean.
///
/// SUCCESS : `true` if key exists and value is a valid boolean.
/// FAILURE : `false`
///
/// TAGS: KvConfig, Get, Bool
///
#define KvConfigGetBool(cfg, key, value)                                                                               \
    _Generic((key), Str *: kvconfig_get_bool_str, Zstr: kvconfig_get_bool_zstr, char *: kvconfig_get_bool_zstr)(       \
        (cfg),                                                                                                         \
        (key),                                                                                                         \
        (value)                                                                                                        \
    )

///
/// Parse and fetch a signed 64-bit integer config value.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Lookup key. Prefer `Str *`; `Zstr` accepted.
/// value[out]  : Parsed integer.
///
/// SUCCESS : `true` if key exists and value is a valid integer.
/// FAILURE : `false`
///
/// TAGS: KvConfig, Get, I64
///
#define KvConfigGetI64(cfg, key, value)                                                                                \
    _Generic((key), Str *: kvconfig_get_i64_str, Zstr: kvconfig_get_i64_zstr, char *: kvconfig_get_i64_zstr)(          \
        (cfg),                                                                                                         \
        (key),                                                                                                         \
        (value)                                                                                                        \
    )

///
/// Parse and fetch a double-precision floating config value.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Lookup key. Prefer `Str *`; `Zstr` accepted.
/// value[out]  : Parsed float.
///
/// SUCCESS : `true` if key exists and value is a valid float.
/// FAILURE : `false`
///
/// TAGS: KvConfig, Get, F64
///
#define KvConfigGetF64(cfg, key, value)                                                                                \
    _Generic((key), Str *: kvconfig_get_f64_str, Zstr: kvconfig_get_f64_zstr, char *: kvconfig_get_f64_zstr)(          \
        (cfg),                                                                                                         \
        (key),                                                                                                         \
        (value)                                                                                                        \
    )

// Snake_case backends live in <Misra/Parsers/KvConfig/Private.h>, pulled
// in from the top of this file so the _Generic dispatch arms above can
// resolve them by name.

#endif // MISRA_PARSERS_KVCONFIG_H
