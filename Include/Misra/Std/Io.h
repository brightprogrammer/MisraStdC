/// file      : std/io.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, All rights reserved.
///
/// formatted reading/writing and other magical stuff

#ifndef MISRA_STD_IO
#define MISRA_STD_IO

#include <Misra/Std/Container.h>
#include <Misra/Types.h>

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
        u8: TO_TYPE_SPECIFIC_IO(u8, &(x)),                                                                             \
        u16: TO_TYPE_SPECIFIC_IO(u16, &(x)),                                                                           \
        u32: TO_TYPE_SPECIFIC_IO(u32, &(x)),                                                                           \
        u64: TO_TYPE_SPECIFIC_IO(u64, &(x)),                                                                           \
        i8: TO_TYPE_SPECIFIC_IO(i8, &(x)),                                                                             \
        i16: TO_TYPE_SPECIFIC_IO(i16, &(x)),                                                                           \
        i32: TO_TYPE_SPECIFIC_IO(i32, &(x)),                                                                           \
        i64: TO_TYPE_SPECIFIC_IO(i64, &(x)),                                                                           \
        const char *: TO_TYPE_SPECIFIC_IO(Zstr, &(x)),                                                                 \
        char *: TO_TYPE_SPECIFIC_IO(Zstr, &(x)),                                                                       \
        default: TO_TYPE_SPECIFIC_IO(UnsupportedType, NULL)                                                            \
    )

///
/// Print out a formatted string with rust-style placeholders
/// to given string "o"
///
/// Takes in TypeSpecificIO structures as arguments. Use FMT(.)
/// to wrap any supported-type variable to it's TypeSpecificIO object.
///
/// o[out]     : Contents appended to this string.
/// fmtstr[in] : Format string with placeholders.
///
void StrWriteFmt(Str *o, const char *fmtstr, ...);

///
/// Parse input string according to format string with rust-style placeholders,
/// extracting values into provided TypeSpecificIO arguments.
///
/// Takes in TypeSpecificIO structures as arguments. Use FMT(.) to wrap any
/// supported-type variable to its TypeSpecificIO object.
///
/// input[in]  : Input string to parse (null-terminated)
/// fmtstr[in] : Format string with placeholders (null-terminated)
/// ...        : Variable arguments of TypeSpecificIO to receive parsed values
///
/// Returns pointer to remaining unparsed input string, or NULL on error.
///
/// Example:
///   const char *input = "Count: 42, Name: Alice";
///   int count;
///   Str name;
///   const char *remaining = StrReadFmt(input, "Count: {}, Name: {}",
///     FMT(&count), FMT(&name));
///
const char *StrReadFmt(const char *input, const char *fmtstr, ...);

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
