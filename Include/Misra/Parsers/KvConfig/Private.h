/// file      : parsers/kvconfig/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Private snake_case backends for the Parsers/KvConfig namespace. Not
/// part of the documented public surface; the PascalCase macros in
/// <Misra/Parsers/KvConfig.h> are the only intended call shape.

#ifndef MISRA_PARSERS_KVCONFIG_PRIVATE_H
#define MISRA_PARSERS_KVCONFIG_PRIVATE_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    // `KvConfig` is `typedef Map(Str, Str) KvConfig` -- defined in the
    // public header before that header pulls this one in.

    Str  kvconfig_get_zstr(KvConfig *cfg, Zstr key);
    Str  kvconfig_get_str(KvConfig *cfg, const Str *key);
    Str  kvconfig_get_cstr(KvConfig *cfg, Zstr key, size len);
    Str *kvconfig_get_ptr_zstr(KvConfig *cfg, Zstr key);
    Str *kvconfig_get_ptr_str(KvConfig *cfg, const Str *key);
    Str *kvconfig_get_ptr_cstr(KvConfig *cfg, Zstr key, size len);
    bool kvconfig_contains_zstr(KvConfig *cfg, Zstr key);
    bool kvconfig_contains_str(KvConfig *cfg, const Str *key);
    bool kvconfig_contains_cstr(KvConfig *cfg, Zstr key, size len);
    bool kvconfig_get_bool_zstr(KvConfig *cfg, Zstr key, bool *value);
    bool kvconfig_get_bool_str(KvConfig *cfg, const Str *key, bool *value);
    bool kvconfig_get_bool_cstr(KvConfig *cfg, Zstr key, size len, bool *value);
    bool kvconfig_get_i64_zstr(KvConfig *cfg, Zstr key, i64 *value);
    bool kvconfig_get_i64_str(KvConfig *cfg, const Str *key, i64 *value);
    bool kvconfig_get_i64_cstr(KvConfig *cfg, Zstr key, size len, i64 *value);
    bool kvconfig_get_f64_zstr(KvConfig *cfg, Zstr key, f64 *value);
    bool kvconfig_get_f64_str(KvConfig *cfg, const Str *key, f64 *value);
    bool kvconfig_get_f64_cstr(KvConfig *cfg, Zstr key, size len, f64 *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_PARSERS_KVCONFIG_PRIVATE_H
