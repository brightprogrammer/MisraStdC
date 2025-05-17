/// file      : std/io.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, All rights reserved.
///
/// formatted reading/writing and other magical stuff

#ifndef MISRA_STD_IO
#define MISRA_STD_IO

#include <Misra/Std/Container.h>
#include <Misra/Types.h>

// c
#include <stdio.h>

typedef struct {
    bool is_hex;
    bool is_caps;
    u8   width;
} FmtInfo;

typedef void (*TypeSpecificWriter)(Str *o, FmtInfo *fmt_info, void *data);
typedef const char *(*TypeSpecificReader)(const char *i, void *data);
typedef struct {
    TypeSpecificWriter writer;
    TypeSpecificReader reader;
    void              *data;
} TypeSpecificIO;

#define TO_TYPE_SPECIFIC_IO(T, d)                                                                                      \
    (TypeSpecificIO) {                                                                                                 \
        .writer = (TypeSpecificWriter)_write_##T, .reader = (TypeSpecificReader)_read_##T, .data = (d)                 \
    }

#define FMT(x)                                                                                                         \
    _Generic(                                                                                                          \
        (x),                                                                                                           \
        Str: TO_TYPE_SPECIFIC_IO(Str, &(x)),                                                                           \
        const char *: TO_TYPE_SPECIFIC_IO(Zstr, &(x)),                                                                 \
        char *: TO_TYPE_SPECIFIC_IO(Zstr, &(x)),                                                                       \
        u8: TO_TYPE_SPECIFIC_IO(u8, &(x)),                                                                             \
        u16: TO_TYPE_SPECIFIC_IO(u16, &(x)),                                                                           \
        u32: TO_TYPE_SPECIFIC_IO(u32, &(x)),                                                                           \
        u64: TO_TYPE_SPECIFIC_IO(u64, &(x)),                                                                           \
        i8: TO_TYPE_SPECIFIC_IO(i8, &(x)),                                                                             \
        i16: TO_TYPE_SPECIFIC_IO(i16, &(x)),                                                                           \
        i32: TO_TYPE_SPECIFIC_IO(i32, &(x)),                                                                           \
        i64: TO_TYPE_SPECIFIC_IO(i64, &(x)),                                                                           \
        default: TO_TYPE_SPECIFIC_IO(UnsupportedType, NULL)                                                            \
    )

///
/// Print out a formatted string with rust-style placeholders
/// to given string "o"
///
/// NOTE: Directly passing literals like FMT(1337) is not supported, especially const char*
/// literals. For constants like integers, booleans, you can use `LVAL(r-value)`
/// to convert an l-value to an r-value an then use in `FMT` like `FMT(LVAL(false))`
///
/// Takes in TypeSpecificIO structures as arguments. Use FMT(.)
/// to wrap any supported-type variable to it's TypeSpecificIO object.
///
/// o[out]     : Contents appended to this string.
/// fmtstr[in] : Format string with placeholders.
/// argv[in]   : Arguments that placeholders will be replaced with.
/// argc[in]   : Number of arguments.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by passed arguments.
/// FAILURE : Does not return, displays log messages.
///
void StrWriteFmtInternal(Str *o, const char *fmtstr, TypeSpecificIO *argv, size argc);

///
/// Parse input string according to format string with rust-style placeholders,
/// extracting values into provided TypeSpecificIO arguments.
///
/// Takes in TypeSpecificIO structures as arguments. Use FMT(.) to wrap any
/// supported-type variable to its TypeSpecificIO object.
///
/// input[in]  : Input string to parse (null-terminated)
/// fmtstr[in] : Format string with placeholders (null-terminated)
/// argv[in]   : Arguments that will be read into from placeholders.
/// argc[in]   : Number of arguments.
///
/// USAGE:
///   const char *input = "Count: 42, Name: Alice";
///   int count;
///   Str name;
///   const char *remaining = StrReadFmt(input, "Count: {}, Name: {}", FMT(count), FMT(name));
///
/// SUCCESS : After reading through `input`, returns back const char* to start reading from (from inside `input`)
/// FAILURE : Does not return, displays log error messages.
///
const char *StrReadFmtInternal(const char *input, const char *fmtstr, TypeSpecificIO *argv, size argc);

///
/// Read formatted data from file streams (stdin, or other file)
///
/// stream[in]
/// fmtstr[in]
/// argv[in]
/// argc[in]
///
/// RETURBN
///
void FReadFmtInternal(FILE *stream, const char *fmtstr, TypeSpecificIO *argv, size argc);

#define StrWriteFmt(out, fmtstr, ...)                                                                                  \
    do {                                                                                                               \
        TypeSpecificIO argv[] = {__VA_ARGS__};                                                                         \
        size           argc   = sizeof(argv) / sizeof(argv[0]);                                                        \
        StrWriteFmtInternal((out), (fmtstr), &argv[0], argc);                                                          \
    } while (0)

#define StrReadFmt(out, fmtstr, ...)                                                                                   \
    do {                                                                                                               \
        TypeSpecificIO argv[] = {__VA_ARGS__};                                                                         \
        size           argc   = sizeof(argv) / sizeof(argv[0]);                                                        \
        StrReadFmtInternal((out), (fmtstr), &argv[0], argc);                                                           \
    } while (0)

#define FWriteFmt(stream, fmtstr, ...)                                                                                 \
    do {                                                                                                               \
        TypeSpecificIO argv[] = {__VA_ARGS__};                                                                         \
        size           argc   = sizeof(argv) / sizeof(argv[0]);                                                        \
        Str            out    = StrInit();                                                                             \
        StrWriteFmtInternal(&out, (fmtstr), &argv[0], argc);                                                           \
        fputs(out.data, (stream));                                                                                     \
    } while (0)

#define WriteFmt(fmtstr, ...) FWriteFmt(stdout, fmtstr, __VA_ARGS__)

#define FReadFmt(file, fmtstr, ...)                                                                                    \
    do {                                                                                                               \
        TypeSpecificIO argv[] = {__VA_ARGS__};                                                                         \
        size           argc   = sizeof(argv) / sizeof(argv[0]);                                                        \
        FReadFmtInternal((file), (fmtstr), &argv[0], argc);                                                            \
    } while (0)

#define ReadFmt(fmtstr, ...) FReadFmt(stdin, fmtstr, __VA_ARGS__)

// not for direct use
void _write_Str(Str *o, FmtInfo *fmt_info, Str *s);
void _write_u8(Str *o, FmtInfo *fmt_info, u8 *v);
void _write_u16(Str *o, FmtInfo *fmt_info, u16 *v);
void _write_u32(Str *o, FmtInfo *fmt_info, u32 *v);
void _write_u64(Str *o, FmtInfo *fmt_info, u64 *v);
void _write_i8(Str *o, FmtInfo *fmt_info, i8 *v);
void _write_i16(Str *o, FmtInfo *fmt_info, i16 *v);
void _write_i32(Str *o, FmtInfo *fmt_info, i32 *v);
void _write_i64(Str *o, FmtInfo *fmt_info, i64 *v);
void _write_Zstr(Str *o, FmtInfo *fmt_info, const char **s);
void _write_UnsupportedType(Str *o, FmtInfo *fmt_info, const char **s);

const char *_read_Str(const char *i, Str *s);
const char *_read_u8(const char *i, u8 *v);
const char *_read_u16(const char *i, u16 *v);
const char *_read_u32(const char *i, u32 *v);
const char *_read_u64(const char *i, u64 *v);
const char *_read_i8(const char *i, i8 *v);
const char *_read_i16(const char *i, i16 *v);
const char *_read_i32(const char *i, i32 *v);
const char *_read_i64(const char *i, i64 *v);
const char *_read_Zstr(const char *i, const char **v);
const char *_read_UnsupportedType(const char *i, const char **s);

#endif // MISRA_STD_IO
