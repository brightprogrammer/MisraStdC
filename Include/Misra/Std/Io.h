/// file      : std/io.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// formatted reading/writing and other magical stuff

#ifndef MISRA_STD_IO
#define MISRA_STD_IO

#include <Misra/Std/Container.h>
#include <Misra/Types.h>

// c
#include <stdio.h>

///
/// Type-specific write callback signature
///
/// TAGS: I/O, Callback, Generic
///
typedef void (*TypeSpecificWriter)(Str *o, FmtInfo *fmt_info, void *data);

///
/// Unified I/O operations container
///
/// TAGS: I/O, Generic, Container
///
typedef const char *(*TypeSpecificReader)(const char *i, void *data);

///
/// Create `TypeSpecificIO` for type T
///
/// T[in] : Type specifier
/// d[in] : Data pointer
///
/// SUCCESS: Returns initialized `TypeSpecificIO ` structure
/// FAILURE: Function cannot fail - compile-time operation
///
/// TAGS: I/O, Macro, TypeConversion, Generic
typedef struct TypeSpecificIO {
    TypeSpecificWriter writer;
    TypeSpecificReader reader;
    void              *data;
} TypeSpecificIO;

#define TO_TYPE_SPECIFIC_IO(T, d)                                                                                      \
    (TypeSpecificIO) {                                                                                                 \
        .writer = (TypeSpecificWriter)_write_##T, .reader = (TypeSpecificReader)_read_##T, .data = (d)                 \
    }

///
/// Type-aware format specifier generator
///
/// x[in] : Value to format
///
/// SUCCESS: Returns `TypeSpecificIO` for supported types
/// FAILURE: Returns unsupported type handler for unknown types
///
/// TAGS: Macro, TypeDispatch, Generic, I/O, Format
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
        f32: TO_TYPE_SPECIFIC_IO(f32, &(x)),                                                                           \
        f64: TO_TYPE_SPECIFIC_IO(f64, &(x)),                                                                           \
        default: TO_TYPE_SPECIFIC_IO(UnsupportedType, NULL)                                                            \
    )

///
/// Print out a formatted string with rust-style placeholders
/// to given string `o`
///
/// WARN: Directly passing literals like `FMT(1337)` is not supported, especially const char*
///       literals. For constants like integers, booleans, you can use `LVAL(r-value)`
///       to convert an l-value to an r-value an then use in `FMT` like `FMT(LVAL(false))`
///
/// Takes in `TypeSpecificIO` structures as arguments. Use `FMT(.)`
/// to wrap any supported-type variable to it's `TypeSpecificIO` object.
///
/// o[out]     : Contents appended to this string.
/// fmtstr[in] : Format string with placeholders.
/// argv[in]   : Arguments that placeholders will be replaced with.
/// argc[in]   : Number of arguments.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by passed arguments.
/// FAILURE : Does not return, displays log messages.
///
/// TAGS: Formatting, I/O, String
///
bool StrWriteFmtInternal(Str* o, const char* fmt, TypeSpecificIO* args, size_t argc);

///
/// Parse input string according to format string with rust-style placeholders,
/// extracting values into provided `TypeSpecificIO` arguments.
///
/// Takes in `TypeSpecificIO` structures as arguments. Use `FMT(.)` to wrap any
/// supported-type variable to its `TypeSpecificIO` object.
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
/// FAILURE : Returns NULL if `fmtstr` does not match with input. In any other case of error, does not return.
///
/// TAGS: Formatting, I/O, Parsing
///
const char *StrReadFmtInternal(const char *input, const char *fmtstr, TypeSpecificIO *argv, size argc);

///
/// Read formatted data from file streams (stdin, or other file)
///
/// stream[in] : `FILE*` we're reading from.
/// fmtstr[in] : Format string to be used for reading. This must exactly describe input format.
/// argv[in]   : Array of `TypeSpecificIO` structures describing where to read for each corresponding placeholder.
/// argc[in]   : Number of `TypeSpecificIO` values in array.
///
/// SUCCESS : Compares fmtstr with stream of characters in `stream` and reads values at placeholders.
///           A valid value will be stored in `FMT(.)` arg provided.
/// FAILURE : Logs out error message and returns. If rollback is possible, then un-reads all the read data.
///           Restoring original state. Method can also abort if something really unexpected is encountered.
///           Returns `NULL` if format string does not match with input `stream`.
///
/// TAGS: Formatting, I/O, File
///
void FReadFmtInternal(FILE *stream, const char *fmtstr, TypeSpecificIO *argv, size argc);

///
/// Print out a formatted string with rust-style placeholders
/// to given string "o". This is a macro wrapper around StrWriteFmtInternal.
///
/// WARN: Directly passing literals like `StrWriteFmt(o, "{}", "literal")` for string literals
///       or `StrWriteFmt(o, "{}", 1337)` for integer literals might not work as expected
///       without proper wrapping using `FMT()`. For constants like integers, booleans,
///       you typically use `FMT(constant_variable)`.
///
/// out[out]    : The Str object to which the formatted string will be appended.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `FMT(variable)`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           result is appended to the `out` Str object.
/// FAILURE : Failure occurs within `StrWriteFmtInternal`. Refer to its documentation
///           for details on failure behavior (typically logs error messages and does not return).
///
/// TAGS: Macro, Wrapper, Format, I/O
///
#define StrWriteFmt(out, fmtstr, ...)                                                                                  \
    do {                                                                                                               \
        TypeSpecificIO argv[] = {__VA_ARGS__};                                                                         \
        size           argc   = sizeof(argv) / sizeof(argv[0]);                                                        \
        StrWriteFmtInternal((out), (fmtstr), &argv[0], argc);                                                          \
    } while (0)

///
/// Parse input string according to format string with rust-style placeholders,
/// extracting values into provided arguments. This is a macro wrapper around StrReadFmtInternal.
///
/// input[in]   : Input string to parse (must be null-terminated).
/// fmtstr[in]  : Format string with `{}` placeholders (must be null-terminated).
/// ...[in]     : Variable number of arguments that will receive the parsed values. Each
///               argument should be a modifiable l-value wrapped with `FMT(&variable)`.
///
/// SUCCESS : Returns a `const char*` pointing to the beginning of the unparsed portion
///           of the `input` string after successful parsing.
/// FAILURE : Failure occurs within `StrReadFmtInternal`. Refer to its documentation
///           for details on failure behavior (typically logs error messages and does not return).
///
/// TAGS: Macro, Wrapper, Format, Parsing, I/O
///
#define StrReadFmt(input, fmtstr, ...)                                                                                 \
    do {                                                                                                               \
        TypeSpecificIO argv[] = {__VA_ARGS__};                                                                         \
        size           argc   = sizeof(argv) / sizeof(argv[0]);                                                        \
        StrReadFmtInternal((input), (fmtstr), &argv[0], argc);                                                         \
    } while (0)

///
/// Read formatted data from a file stream. This is a macro wrapper around FReadFmtInternal.
///
/// stream[in]  : Pointer to the `FILE` stream to read from.
/// fmtstr[in]  : Format string to be used for reading. This must exactly describe the
///               expected input format in the stream.
/// ...[in]     : Variable number of arguments that will receive the read values. Each
///               argument should be a modifiable l-value wrapped with `FMT(&variable)`.
///
/// SUCCESS : Attempts to match `fmtstr` with the stream of characters in `stream` and
///           reads values into the provided arguments wrapped with `FMT()`.
/// FAILURE : Failure occurs within `FReadFmtInternal`. Refer to its documentation for
///           details on failure behavior (logs error message and returns, may rollback
///           read data, or abort in unexpected situations).
///
/// TAGS: Macro, Wrapper, File, I/O
///
#define FReadFmt(file, fmtstr, ...)                                                                                    \
    do {                                                                                                               \
        TypeSpecificIO argv[] = {__VA_ARGS__};                                                                         \
        size           argc   = sizeof(argv) / sizeof(argv[0]);                                                        \
        FReadFmtInternal((file), (fmtstr), &argv[0], argc);                                                            \
    } while (0)

///
/// Write formatted output to a file stream. This macro internally uses StrWriteFmtInternal
/// to format the string and then writes it to the stream.
///
/// stream[in]  : Pointer to the `FILE` stream to write to.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `FMT(variable)`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string is written to the specified `stream`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to the stream (handled by `fputs`). Errors
///           from `StrWriteFmtInternal` (logging messages) might also occur.
///
/// TAGS: Macro, Wrapper, File, I/O
///
#define FWriteFmt(stream, fmtstr, ...)                                                                                 \
    do {                                                                                                               \
        TypeSpecificIO argv[] = {__VA_ARGS__};                                                                         \
        size           argc   = sizeof(argv) / sizeof(argv[0]);                                                        \
        Str            out    = StrInit();                                                                             \
        StrWriteFmtInternal(&out, (fmtstr), &argv[0], argc);                                                           \
        fputs(out.data, (stream));                                                                                     \
    } while (0)

///
/// Write formatted output to a file stream followed by a newline character.
/// This macro internally uses StrWriteFmtInternal to format the string and then writes
/// it to the stream followed by a newline.
///
/// stream[in]  : Pointer to the `FILE` stream to write to.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `FMT(variable)`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string followed by a newline is written to the `stream`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to the stream (`fputs` or `fputc`). Errors
///           from `StrWriteFmtInternal` (logging messages) might also occur.
///
/// TAGS: Macro, Wrapper, File, I/O
///
#define FWriteFmtLn(stream, fmtstr, ...)                                                                               \
    do {                                                                                                               \
        TypeSpecificIO argv[] = {__VA_ARGS__};                                                                         \
        size           argc   = sizeof(argv) / sizeof(argv[0]);                                                        \
        Str            out    = StrInit();                                                                             \
        StrWriteFmtInternal(&out, (fmtstr), &argv[0], argc);                                                           \
        fputs(out.data, (stream));                                                                                     \
        fputc('\n', (stream));                                                                                         \
    } while (0)

///
/// Write formatted output to the standard output stream (`stdout`).
/// This is a convenience macro calling FWriteFmt with `stdout`.
///
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `FMT(variable)`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string is written to `stdout`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to `stdout` (handled by `fputs`). Errors
///           from `StrWriteFmtInternal` (logging messages) might also occur.
///
/// TAGS: Macro, Convenience, Stdout, I/O
///
#define WriteFmt(fmtstr, ...) FWriteFmt(stdout, fmtstr, __VA_ARGS__)

///
/// Write formatted output to the standard output stream (`stdout`) followed by a newline.
/// This is a convenience macro calling FWriteFmtLn with `stdout`.
///
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `FMT(variable)`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string followed by a newline is written to `stdout`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to `stdout` (`fputs` or `fputc`). Errors
///           from `StrWriteFmtInternal` (logging messages) might also occur.
///
/// TAGS: Macro, Convenience, Stdout, I/O
///
#define WriteFmtLn(fmtstr, ...) FWriteFmtLn(stdout, fmtstr, __VA_ARGS__)

///
/// Read formatted input from the standard input stream (`stdin`).
/// This is a convenience macro calling FReadFmt with `stdin`.
///
/// fmtstr[in]  : Format string to be used for reading. This must exactly describe the
///               expected input format from `stdin`.
/// ...[in]     : Variable number of arguments that will receive the read values. Each
///               argument should be a modifiable l-value wrapped with `FMT(&variable)`.
///
/// SUCCESS : Attempts to match `fmtstr` with the input from `stdin` and reads values
///           into the provided arguments wrapped with `FMT()`.
/// FAILURE : Failure occurs within `FReadFmtInternal`. Refer to its documentation for
///           details on failure behavior (logs error message and returns, may rollback
///           read data, or abort in unexpected situations).
///
/// TAGS: Macro, Convenience, Stdin, I/O
///
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
void _write_f32(Str *o, FmtInfo *fmt_info, f32 *v);
void _write_f64(Str *o, FmtInfo *fmt_info, f64 *v);

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
const char *_read_f32(const char *i, f32 *v);
const char *_read_f64(const char *i, f64 *v);

#endif // MISRA_STD_IO
