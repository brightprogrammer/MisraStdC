/// file      : std/container/str/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Snake_case backends behind the public Str ops macros.
/// These are the runtime bodies the `Str*` PascalCase macros in
/// `Ops.h` dispatch to. Do not call them directly from outside the
/// `Str` implementation -- the macros are the public surface.

#ifndef MISRA_STD_CONTAINER_STR_PRIVATE_H
#define MISRA_STD_CONTAINER_STR_PRIVATE_H

#include "Type.h"
#include <Misra/Std/Zstr.h>

#ifdef __cplusplus
extern "C" {
#endif

    size str_index_of_str (const Str *s, const Str *key);
    size str_index_of_zstr(const Str *s, Zstr key);
    size str_index_of_cstr(const Str *s, Zstr key, size key_len);

    bool str_contains_str (const Str *s, const Str *key);
    bool str_contains_zstr(const Str *s, Zstr key);
    bool str_contains_cstr(const Str *s, Zstr key, size key_len);

    bool str_starts_with_str (const Str *s, const Str *prefix);
    bool str_starts_with_zstr(const Str *s, Zstr prefix);
    bool str_starts_with_cstr(const Str *s, Zstr prefix, size prefix_len);

    bool str_ends_with_str (const Str *s, const Str *suffix);
    bool str_ends_with_zstr(const Str *s, Zstr suffix);
    bool str_ends_with_cstr(const Str *s, Zstr suffix, size suffix_len);

    void str_replace_str (Str *s, const Str *match, const Str *replacement, size count);
    void str_replace_zstr(Str *s, Zstr match, Zstr replacement, size count);
    void str_replace_cstr(
        Str *s,
        Zstr match,
        size match_len,
        Zstr replacement,
        size replacement_len,
        size count
    );

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_PRIVATE_H
