/// file      : parsers/kvconfig.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Simple key-value configuration parser.
///

#ifndef MISRA_PARSERS_KVCONFIG_H
#define MISRA_PARSERS_KVCONFIG_H

#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

///
/// Key-value configuration map.
///
/// Duplicate keys are resolved with last-write-wins semantics during parsing.
///
/// USAGE:
///   KvConfig cfg = KvConfigInit();
///   Str text = StrInitFromZstr("host = localhost\nport = 8080\n");
///   StrIter si = KvConfigParse(StrIterFromStr(text), &cfg);
///
///   Str *host_ptr = KvConfigGetPtr(&cfg, "host");
///   Str  host     = KvConfigGet(&cfg, "host");
///   i64  port = 0;
///   KvConfigGetI64(&cfg, "port", &port);
///   StrDeinit(&host);
///
/// FIELDS:
/// - inherited `Map(Str, Str)` fields from the underlying map storage.
///
/// TAGS: Parser, Config, KeyValue, Map
///
typedef Map(Str, Str) KvConfig;

///
/// Initialize a `KvConfig` object with deep-copy ownership for both keys and values.
///
/// allocator_ptr : Allocator that owns the config's storage. May be a
///                 typed allocator handle (`&heap`) or a raw `Allocator *`.
///
/// SUCCESS : Returns initialized config object.
///
#define KvConfigInit(...) MISRA_OVERLOAD(KvConfigInit, __VA_ARGS__)
#define KvConfigInit_0()  KvConfigInit_1(MisraScope)
#define KvConfigInit_1(allocator_ptr)                                                                                  \
    MapInitFull_9(                                                                                                     \
        KvConfigHash,                                                                                                  \
        KvConfigCompare,                                                                                               \
        KvConfigCompare,                                                                                               \
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
/// Hash a `Str` for use as config key.
///
/// data[in] : Pointer to `Str`.
/// size[in] : Ignored. Included for generic hash compatibility.
///
/// SUCCESS : Stable hash of the string bytes.
///
u64 KvConfigHash(const void *data, u32 size);

///
/// Compare two `Str` values for config key/value equality.
///
/// lhs[in] : Pointer to left `Str`.
/// rhs[in] : Pointer to right `Str`.
///
/// SUCCESS : `0` if equal, negative if lhs < rhs, positive if lhs > rhs.
///
i32 KvConfigCompare(const void *lhs, const void *rhs);

///
/// Skip horizontal whitespace in config text.
///
/// This skips `' '`, `'\t'`, and `'\r'`, but not `'\n'`.
///
/// si[in] : Iterator to advance.
///
/// SUCCESS : Returns updated iterator.
///
StrIter KvConfigSkipWhitespace(StrIter si);

///
/// Skip current line including trailing newline when present.
///
/// si[in] : Iterator positioned anywhere on a line.
///
/// SUCCESS : Returns iterator positioned at the first character of the next line.
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
StrIter KvConfigParse(StrIter si, KvConfig *cfg);

///
/// Get stored value for `key` as a new `Str` copy.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Zero-terminated key string.
///
/// SUCCESS : Newly allocated copy of stored `Str` value. Caller must `StrDeinit(...)` it.
/// FAILURE : Empty `Str` if key does not exist.
///
Str kvconfig_get_zstr(KvConfig *cfg, Zstr key);
Str kvconfig_get_str(KvConfig *cfg, const Str *key);
#define KvConfigGet(cfg, key)                                                                                                            \
    _Generic((key), Str *: kvconfig_get_str, const Str *: kvconfig_get_str, char *: kvconfig_get_zstr, const char *: kvconfig_get_zstr)( \
        (cfg),                                                                                                                           \
        (key)                                                                                                                            \
    )

///
/// Get stored value for `key` by internal reference.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Zero-terminated key string.
///
/// SUCCESS : Pointer to stored `Str` value. Do not deinitialize or mutate through ownership-sensitive APIs.
/// FAILURE : `NULL` if key does not exist.
///
Str *kvconfig_get_ptr_zstr(KvConfig *cfg, Zstr key);
Str *kvconfig_get_ptr_str(KvConfig *cfg, const Str *key);
#define KvConfigGetPtr(cfg, key)                                                                                                                         \
    _Generic((key), Str *: kvconfig_get_ptr_str, const Str *: kvconfig_get_ptr_str, char *: kvconfig_get_ptr_zstr, const char *: kvconfig_get_ptr_zstr)( \
        (cfg),                                                                                                                                           \
        (key)                                                                                                                                            \
    )

///
/// Check whether a key exists in config.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Zero-terminated key string.
///
/// SUCCESS : `true` if key exists.
/// FAILURE : `false`
///
bool kvconfig_contains_zstr(KvConfig *cfg, Zstr key);
bool kvconfig_contains_str(KvConfig *cfg, const Str *key);
#define KvConfigContains(cfg, key)                                                                                                                           \
    _Generic((key), Str *: kvconfig_contains_str, const Str *: kvconfig_contains_str, char *: kvconfig_contains_zstr, const char *: kvconfig_contains_zstr)( \
        (cfg),                                                                                                                                               \
        (key)                                                                                                                                                \
    )

///
/// Parse and fetch a boolean config value.
///
/// Accepted values: `true`, `false`, `yes`, `no`, `on`, `off`, `1`, `0`.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Zero-terminated key string.
/// value[out]  : Parsed boolean.
///
/// SUCCESS : `true` if key exists and value is a valid boolean.
/// FAILURE : `false`
///
bool kvconfig_get_bool_zstr(KvConfig *cfg, Zstr key, bool *value);
bool kvconfig_get_bool_str(KvConfig *cfg, const Str *key, bool *value);
#define KvConfigGetBool(cfg, key, value)                                                                                                                     \
    _Generic((key), Str *: kvconfig_get_bool_str, const Str *: kvconfig_get_bool_str, char *: kvconfig_get_bool_zstr, const char *: kvconfig_get_bool_zstr)( \
        (cfg),                                                                                                                                               \
        (key),                                                                                                                                               \
        (value)                                                                                                                                              \
    )

///
/// Parse and fetch a signed 64-bit integer config value.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Zero-terminated key string.
/// value[out]  : Parsed integer.
///
/// SUCCESS : `true` if key exists and value is a valid integer.
/// FAILURE : `false`
///
bool kvconfig_get_i64_zstr(KvConfig *cfg, Zstr key, i64 *value);
bool kvconfig_get_i64_str(KvConfig *cfg, const Str *key, i64 *value);
#define KvConfigGetI64(cfg, key, value)                                                                                                                  \
    _Generic((key), Str *: kvconfig_get_i64_str, const Str *: kvconfig_get_i64_str, char *: kvconfig_get_i64_zstr, const char *: kvconfig_get_i64_zstr)( \
        (cfg),                                                                                                                                           \
        (key),                                                                                                                                           \
        (value)                                                                                                                                          \
    )

///
/// Parse and fetch a double-precision floating config value.
///
/// cfg[in,out] : Parsed config.
/// key[in]     : Zero-terminated key string.
/// value[out]  : Parsed float.
///
/// SUCCESS : `true` if key exists and value is a valid float.
/// FAILURE : `false`
///
bool kvconfig_get_f64_zstr(KvConfig *cfg, Zstr key, f64 *value);
bool kvconfig_get_f64_str(KvConfig *cfg, const Str *key, f64 *value);
#define KvConfigGetF64(cfg, key, value)                                                                                                                  \
    _Generic((key), Str *: kvconfig_get_f64_str, const Str *: kvconfig_get_f64_str, char *: kvconfig_get_f64_zstr, const char *: kvconfig_get_f64_zstr)( \
        (cfg),                                                                                                                                           \
        (key),                                                                                                                                           \
        (value)                                                                                                                                          \
    )

#endif // MISRA_PARSERS_KVCONFIG_H
