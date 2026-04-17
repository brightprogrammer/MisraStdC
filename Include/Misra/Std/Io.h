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

// REF : https://devblogs.microsoft.com/cppblog/announcing-full-support-for-a-c-c-conformant-preprocessor-in-msvc/
#if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
#    error "I need /Zc:prerocessor flag enabled in MSVC to compile IO code correctly"
#endif

///
/// Defines text alignment options for formatted output.
///
/// ALIGN_LEFT   : Left-aligned text.
/// ALIGN_RIGHT  : Right-aligned text.
/// ALIGN_CENTER : Center-aligned text.
///
/// TAGS: Formatting, Alignment, Text
typedef enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,

    ENDIAN_NATIVE = ALIGN_CENTER,
    ENDIAN_LITTLE = ALIGN_LEFT,
    ENDIAN_BIG    = ALIGN_RIGHT
} Alignment, Endinanness;

///
/// Format flags for text output.
///
/// FMT_FLAG_NONE          : No special formatting.
/// FMT_FLAG_CHAR          : Format as character.
/// FMT_FLAG_HEX           : Format as hexadecimal.
/// FMT_FLAG_BINARY        : Format as binary.
/// FMT_FLAG_OCTAL         : Format as octal.
/// FMT_FLAG_SCIENTIFIC    : Scientific notation for floats.
/// FMT_FLAG_CAPS          : Use capital letters for hex/scientific.
/// FMT_FLAG_FORCE_CASE    : Force case conversion (used with FMT_FLAG_CAPS)
/// FMT_FLAG_HAS_PRECISION : Precision was specified in format string.
/// FMT_FLAG_RAW           : Read/write data in raw binary format
/// FMT_FLAG_STRING        : Read a single word, a quoted string (single or double quoted)
///
/// TAGS: Formatting, Text, Flags
typedef enum {
    FMT_FLAG_NONE          = 0,
    FMT_FLAG_CHAR          = 1 << 0,
    FMT_FLAG_HEX           = 1 << 1,
    FMT_FLAG_BINARY        = 1 << 2,
    FMT_FLAG_OCTAL         = 1 << 3,
    FMT_FLAG_SCIENTIFIC    = 1 << 4,
    FMT_FLAG_CAPS          = 1 << 5,
    FMT_FLAG_FORCE_CASE    = 1 << 6,
    FMT_FLAG_HAS_PRECISION = 1 << 7,
    FMT_FLAG_RAW           = 1 << 8,
    FMT_FLAG_STRING        = 1 << 9,
} FormatFlagsBits;
typedef u32 FormatFlags;

///
/// Stores formatting information for text output.
///
/// align          : Text alignment (left, right, center).
/// width          : Minimum field width.
/// precision      : Number of decimal places for floating point.
/// flags          : Format flags (see FormatFlags enum).
///
/// TAGS: Formatting, Text, Configuration
typedef struct {
    union {
        Alignment   align;
        Endinanness endian;
    };
    u32         width; /// Alignment width or raw read/write size
    u32         precision;
    FormatFlags flags;
    u32         max_read_len;
} FmtInfo;

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
typedef const char *(*TypeSpecificReader)(const char *i, FmtInfo *fmt_info, void *data);

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

#ifdef __cplusplus
#    define EMPTY_TYPE_SPECIFIC_IO() (TypeSpecificIO {NULL, NULL, NULL})
#else
#    define EMPTY_TYPE_SPECIFIC_IO() ((TypeSpecificIO) {NULL, NULL, NULL})
#endif

static inline TypeSpecificIO TO_TYPE_SPECIFIC_IO_IMPL(TypeSpecificWriter w, TypeSpecificReader r, void *d) {
    return (TypeSpecificIO) {.writer = w, .reader = r, .data = d};
}

#define TO_TYPE_SPECIFIC_IO(T, d)                                                                                      \
    TO_TYPE_SPECIFIC_IO_IMPL((TypeSpecificWriter)_write_##T, (TypeSpecificReader)_read_##T, (d))

#if defined(_MSC_VER) || defined(__MSC_VER)
#    define IOFMT(x)                                                                                                   \
        _Generic(                                                                                                      \
            (x),                                                                                                       \
            TypeSpecificIO: (x),                                                                                       \
            Str: TO_TYPE_SPECIFIC_IO(Str, &(x)),                                                                       \
            Float: TO_TYPE_SPECIFIC_IO(Float, &(x)),                                                                   \
            Int: TO_TYPE_SPECIFIC_IO(Int, &(x)),                                                                       \
            BitVec: TO_TYPE_SPECIFIC_IO(BitVec, &(x)),                                                                 \
            const char *: TO_TYPE_SPECIFIC_IO(Zstr, &(x)),                                                             \
            char *: TO_TYPE_SPECIFIC_IO(Zstr, &(x)),                                                                   \
            unsigned char: TO_TYPE_SPECIFIC_IO(u8, &(x)),                                                              \
            unsigned short: TO_TYPE_SPECIFIC_IO(u16, &(x)),                                                            \
            unsigned int: TO_TYPE_SPECIFIC_IO(u32, &(x)),                                                              \
            unsigned long: sizeof(unsigned long) == 4 ? TO_TYPE_SPECIFIC_IO(u32, &(x)) :                               \
                                                        TO_TYPE_SPECIFIC_IO(u64, &(x)),                                \
            unsigned long long: TO_TYPE_SPECIFIC_IO(u64, &(x)),                                                        \
            signed char: TO_TYPE_SPECIFIC_IO(i8, &(x)),                                                                \
            signed short: TO_TYPE_SPECIFIC_IO(i16, &(x)),                                                              \
            signed int: TO_TYPE_SPECIFIC_IO(i32, &(x)),                                                                \
            signed long: sizeof(signed long) == 4 ? TO_TYPE_SPECIFIC_IO(i32, &(x)) : TO_TYPE_SPECIFIC_IO(i64, &(x)),   \
            signed long long: TO_TYPE_SPECIFIC_IO(i64, &(x)),                                                          \
            f32: TO_TYPE_SPECIFIC_IO(f32, &(x)),                                                                       \
            f64: TO_TYPE_SPECIFIC_IO(f64, &(x)),                                                                       \
            char: TO_TYPE_SPECIFIC_IO(i8, &(x)),                                                                       \
            default: TO_TYPE_SPECIFIC_IO(UnsupportedType, NULL)                                                        \
        )
#else
///
/// Type-aware format specifier generator
///
/// x[in] : Value to format
///
/// SUCCESS: Returns `TypeSpecificIO` for supported types
/// FAILURE: Returns unsupported type handler for unknown types
///
/// TAGS: Macro, TypeDispatch, Generic, I/O, Format
#    define IOFMT(x)                                                                                                   \
        _Generic(                                                                                                      \
            (x),                                                                                                       \
            TypeSpecificIO: (x),                                                                                       \
            Str: TO_TYPE_SPECIFIC_IO(Str, (void *)&(x)),                                                               \
            Float: TO_TYPE_SPECIFIC_IO(Float, (void *)&(x)),                                                           \
            Int: TO_TYPE_SPECIFIC_IO(Int, (void *)&(x)),                                                               \
            BitVec: TO_TYPE_SPECIFIC_IO(BitVec, (void *)&(x)),                                                         \
            const char *: TO_TYPE_SPECIFIC_IO(Zstr, (void *)&(x)),                                                     \
            char *: TO_TYPE_SPECIFIC_IO(Zstr, (void *)&(x)),                                                           \
            unsigned char: TO_TYPE_SPECIFIC_IO(u8, (void *)&(x)),                                                      \
            unsigned short: TO_TYPE_SPECIFIC_IO(u16, (void *)&(x)),                                                    \
            unsigned int: TO_TYPE_SPECIFIC_IO(u32, (void *)&(x)),                                                      \
            unsigned long: sizeof(unsigned long) == 4 ? TO_TYPE_SPECIFIC_IO(u32, (void *)&(x)) :                       \
                                                        TO_TYPE_SPECIFIC_IO(u64, (void *)&(x)),                        \
            unsigned long long: TO_TYPE_SPECIFIC_IO(u64, (void *)&(x)),                                                \
            signed char: TO_TYPE_SPECIFIC_IO(i8, (void *)&(x)),                                                        \
            signed short: TO_TYPE_SPECIFIC_IO(i16, (void *)&(x)),                                                      \
            signed int: TO_TYPE_SPECIFIC_IO(i32, (void *)&(x)),                                                        \
            signed long: sizeof(signed long) == 4 ? TO_TYPE_SPECIFIC_IO(i32, (void *)&(x)) :                           \
                                                    TO_TYPE_SPECIFIC_IO(i64, (void *)&(x)),                            \
            signed long long: TO_TYPE_SPECIFIC_IO(i64, (void *)&(x)),                                                  \
            f32: TO_TYPE_SPECIFIC_IO(f32, (void *)&(x)),                                                               \
            f64: TO_TYPE_SPECIFIC_IO(f64, (void *)&(x)),                                                               \
            char: TO_TYPE_SPECIFIC_IO(i8, (void *)&(x)),                                                               \
            default: TO_TYPE_SPECIFIC_IO(UnsupportedType, NULL)                                                        \
        )
#endif

///
/// Print out a formatted string with rust-style placeholders
/// to given string `o`.
///
/// WARN: Directly passing literals like `1337` is not supported, especially const char*
///       literals. For constants like integers, booleans, you can use `LVAL(r-value)`
///       to convert an l-value to an r-value an then use in `FMT` like `LVAL(false)`
///
/// Takes in `TypeSpecificIO` structures as arguments. Use `.`
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
bool StrWriteFmtInternal(Str *o, const char *fmt, TypeSpecificIO *args, u64 argc);

///
/// Parse input string according to format string with rust-style placeholders,
/// extracting values into provided `TypeSpecificIO` arguments.
///
/// Takes in `TypeSpecificIO` structures as arguments. Use `.` to wrap any
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
///   const char *remaining = StrReadFmt(input, "Count: {}, Name: {}", count, name);
///
/// SUCCESS : After reading through `input`, returns back const char* to start reading from (from inside `input`)
/// FAILURE : Returns NULL if `fmtstr` does not match with input. In any other case of error, does not return.
///
/// TAGS: Formatting, I/O, Parsing
///
const char *StrReadFmtInternal(const char *input, const char *fmtstr, TypeSpecificIO *argv, u64 argc);

///
/// Read formatted data from file streams (stdin, or other file)
///
/// stream[in] : `FILE*` we're reading from.
/// fmtstr[in] : Format string to be used for reading. This must exactly describe input format.
/// argv[in]   : Array of `TypeSpecificIO` structures describing where to read for each corresponding placeholder.
/// argc[in]   : Number of `TypeSpecificIO` values in array.
///
/// SUCCESS : Compares fmtstr with stream of characters in `stream` and reads values at placeholders.
///           A valid value will be stored in `.` arg provided.
/// FAILURE : Logs out error message and returns. If rollback is possible, then un-reads all the read data.
///           Restoring original state. Method can also abort if something really unexpected is encountered.
///           Returns `NULL` if format string does not match with input `stream`.
///
/// TAGS: Formatting, I/O, File
///
void FReadFmtInternal(FILE *stream, const char *fmtstr, TypeSpecificIO *argv, u64 argc);

///
/// Helper macro to append a comma after wrapping given argument in IOFMT
/// Used in following macros
///
#define IOFMT_LVAL_APPEND_COMMA(x) IOFMT(LVAL(x)),
#define IOFMT_APPEND_COMMA(x)      IOFMT(x),

///
/// Print out a formatted string with rust-style placeholders
/// to given string `o`. This is a macro wrapper around StrWriteFmtImpl.
///
/// WARN: Directly passing literals like `StrWriteFmt(o, "{}", "literal")` for string literals
///       or `StrWriteFmt(o, "{}", 1337)` for integer literals might not work as expected
///       without proper wrapping using ``. For constants like integers, booleans,
///       you typically use `constant_variable`.
/// NOTE: New content is appended at the end of given Str object.
///
/// out[out]    : The Str object to which the formatted string will be appended.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `variable`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           result is appended to the `out` Str object.
/// FAILURE : Failure occurs within `StrWriteFmtInternal`. Refer to its documentation
///           for details on failure behavior (typically logs error messages and does not return).
///
/// TAGS: Macro, Wrapper, Format, I/O
///
// #define StrWriteFmt(out, ...) StrWriteFmtImpl(out, __VA_ARGS__, (TypeSpecificIO) {NULL, NULL, NULL})
#define StrWriteFmt(out, ...) StrWriteFmt_IMPL1(out, __VA_ARGS__)
#define StrWriteFmt_IMPL1(input, fmtstr, ...)                                                                          \
    StrWriteFmt_IMPL2(                                                                                                 \
        input,                                                                                                         \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define StrWriteFmt_IMPL2(input, fmtstr, varr)                                                                         \
    do {                                                                                                               \
        TypeSpecificIO *argv_ = &(varr)[0];                                                                            \
        u64             argc_ = sizeof(varr) / sizeof(TypeSpecificIO);                                                 \
        StrWriteFmtInternal((input), (fmtstr), argv_, argc_ - 1);                                                      \
    } while (0)

///
/// Parse input string according to format string with rust-style placeholders,
/// extracting values into provided arguments. This is a macro wrapper around StrReadFmtInternal.
///
/// NOTE: Provided input string must be an assignable l-value. The macro automatically updates given
///       input string to new parse position after a successful parse. If parse fails, the input string
///       pointers does not change.
///
/// WARN: Do not free given input after use. Pointer value is changed after successful read. It can be
///       like `(input) + 1` or `(input + 1233493783847394)` which are invalid pointers to be called `FREE` upon.
///
/// WARN: Not providing an assingable input (first parameter) will result in undefined behavior.
///       If you're lucky you'll get a segfault.
///
/// INFO: The new `input` value after a successful read will be in [`input`, `input + len(input)`]
///
/// USAGE:
///    struct {i32 id; Str name} ParseInput(const char* i, const char** o) {
///        const char* p = i; // create a new variable to pass the pointer
///
///        i32 id; Str name = StrInit();
///        StrReadFmt(p, "Person id = {} and name = {}", id, name);
///
///        *o = p; // position after parsed input
///    }
///
/// input[in]   : Input string to parse (must be null-terminated).
/// fmtstr[in]  : Format string with `{}` placeholders (must be null-terminated).
/// ...[in]     : Variable number of arguments that will receive the parsed values. Each
///               argument should be a modifiable l-value wrapped with `&variable`.
///
/// SUCCESS : Returns a `const char*` pointing to the beginning of the unparsed portion
///           of the `input` string after successful parsing.
/// FAILURE : Failure occurs within `StrReadFmtInternal`. Refer to its documentation
///           for details on failure behavior (typically logs error messages and does not return).
///
/// TAGS: Macro, Wrapper, Format, Parsing, I/O
///
#define StrReadFmt(input, ...) StrReadFmt_IMPL1(input, __VA_ARGS__)
#define StrReadFmt_IMPL1(input, fmtstr, ...)                                                                           \
    StrReadFmt_IMPL2(                                                                                                  \
        input,                                                                                                         \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                                    \
    })                                                                                                             \
    )
#define StrReadFmt_IMPL2(input, fmtstr, varr)                                                                          \
    do {                                                                                                               \
        TypeSpecificIO *argv_     = &(varr)[0];                                                                        \
        char          **_p_input_ = (char **)(&(input));                                                               \
        u64             argc_     = sizeof(varr) / sizeof(TypeSpecificIO);                                             \
        const char     *_input_   = StrReadFmtInternal((const char *)*(_p_input_), (fmtstr), argv_, argc_ - 1);        \
        (*_p_input_)              = (char *)(_input_) ? (char *)(_input_) : (*_p_input_);                              \
    } while (0)

///
/// Read formatted data from a file stream. This is a macro wrapper around FReadFmtInternal.
///
/// stream[in]  : Pointer to the `FILE` stream to read from.
/// fmtstr[in]  : Format string to be used for reading. This must exactly describe the
///               expected input format in the stream.
/// ...[in]     : Variable number of arguments that will receive the read values. Each
///               argument should be a modifiable l-value wrapped with `&variable`.
///
/// SUCCESS : Attempts to match `fmtstr` with the stream of characters in `stream` and
///           reads values into the provided arguments wrapped with ``.
/// FAILURE : Failure occurs within `FReadFmtInternal`. Refer to its documentation for
///           details on failure behavior (logs error message and returns, may rollback
///           read data, or abort in unexpected situations).
///
/// TAGS: Macro, Wrapper, File, I/O
///
#define FReadFmt(file, ...) FReadFmt_IMPL1(file, __VA_ARGS__)
#define FReadFmt_IMPL1(file, fmtstr, ...)                                                                              \
    FReadFmt_IMPL2(                                                                                                    \
        file,                                                                                                          \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                                    \
    })                                                                                                             \
    )
#define FReadFmt_IMPL2(file, fmtstr, varr)                                                                             \
    do {                                                                                                               \
        TypeSpecificIO *argv_ = &(varr)[0];                                                                            \
        u64             argc_ = sizeof(varr) / sizeof(TypeSpecificIO) - 1;                                             \
        FReadFmtInternal((file), (fmtstr), argv_, argc_);                                                              \
    } while (0)

///
/// Write formatted output to a file stream. This macro internally uses StrWriteFmtInternal
/// to format the string and then writes it to the stream.
///
/// stream[in]  : Pointer to the `FILE` stream to write to.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `variable`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string is written to the specified `stream`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to the stream (handled by `fputs`). Errors
///           from `StrWriteFmtInternal` (logging messages) might also occur.
///
/// TAGS: Macro, Wrapper, File, I/O
///
#define FWriteFmt(stream, ...) FWriteFmt_IMPL1(stream, __VA_ARGS__)
#define FWriteFmt_IMPL1(stream, fmtstr, ...)                                                                           \
    FWriteFmt_IMPL2(                                                                                                   \
        stream,                                                                                                        \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define FWriteFmt_IMPL2(stream, fmtstr, varr)                                                                          \
    do {                                                                                                               \
        TypeSpecificIO *argv_ = &(varr)[0];                                                                            \
        u64             argc_ = sizeof(varr) / sizeof(TypeSpecificIO) - 1;                                             \
        Str             out_  = StrInit();                                                                             \
        StrWriteFmtInternal(&out_, (fmtstr), argv_, argc_);                                                            \
        fwrite(out_.data, 1, out_.length, (stream));                                                                   \
        fflush(stream);                                                                                                \
        StrDeinit(&out_);                                                                                              \
    } while (0)

///
/// Write formatted output to a file stream followed by a newline character.
/// This macro internally uses StrWriteFmtInternal to format the string and then writes
/// it to the stream followed by a newline.
///
/// stream[in]  : Pointer to the `FILE` stream to write to.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `variable`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string followed by a newline is written to the `stream`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to the stream (`fputs` or `fputc`). Errors
///           from `StrWriteFmtInternal` (logging messages) might also occur.
///
/// TAGS: Macro, Wrapper, File, I/O
///
#define FWriteFmtLn(stream, ...) FWriteFmtLn_IMPL1(stream, __VA_ARGS__)
#define FWriteFmtLn_IMPL1(stream, fmtstr, ...)                                                                         \
    FWriteFmtLn_IMPL2(                                                                                                 \
        stream,                                                                                                        \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define FWriteFmtLn_IMPL2(stream, fmtstr, varr)                                                                        \
    do {                                                                                                               \
        TypeSpecificIO *argv_ = &(varr)[0];                                                                            \
        u64             argc_ = sizeof(varr) / sizeof(TypeSpecificIO) - 1;                                             \
        Str             out_  = StrInit();                                                                             \
        StrWriteFmtInternal(&out_, (fmtstr), argv_, argc_);                                                            \
        fwrite(out_.data, 1, out_.length, (stream));                                                                   \
        fputc('\n', (stream));                                                                                         \
        fflush(stream);                                                                                                \
        StrDeinit(&out_);                                                                                              \
    } while (0)

///
/// Write formatted output to the standard output stream (`stdout`).
/// This is a convenience macro calling FWriteFmt with `stdout`.
///
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `variable`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string is written to `stdout`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to `stdout` (handled by `fputs`). Errors
///           from `StrWriteFmtInternal` (logging messages) might also occur.
///
/// TAGS: Macro, Convenience, Stdout, I/O
///
#define WriteFmt(...) FWriteFmt(stdout, __VA_ARGS__)

///
/// Write formatted output to the standard output stream (`stdout`) followed by a newline.
/// This is a convenience macro calling FWriteFmtLn with `stdout`.
///
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `variable`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string followed by a newline is written to `stdout`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to `stdout` (`fputs` or `fputc`). Errors
///           from `StrWriteFmtInternal` (logging messages) might also occur.
///
/// TAGS: Macro, Convenience, Stdout, I/O
///
#define WriteFmtLn(...) FWriteFmtLn(stdout, __VA_ARGS__)

///
/// Read formatted input from the standard input stream (`stdin`).
/// This is a convenience macro calling FReadFmt with `stdin`.
///
/// fmtstr[in]  : Format string to be used for reading. This must exactly describe the
///               expected input format from `stdin`.
/// ...[in]     : Variable number of arguments that will receive the read values. Each
///               argument should be a modifiable l-value wrapped with `&variable`.
///
/// SUCCESS : Attempts to match `fmtstr` with the input from `stdin` and reads values
///           into the provided arguments wrapped with ``.
/// FAILURE : Failure occurs within `FReadFmtInternal`. Refer to its documentation for
///           details on failure behavior (logs error message and returns, may rollback
///           read data, or abort in unexpected situations).
///
/// TAGS: Macro, Convenience, Stdin, I/O
///
#define ReadFmt(...) FReadFmt(stdin, __VA_ARGS__)

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
void _write_Float(Str *o, FmtInfo *fmt_info, Float *value);
void _write_BitVec(Str *o, FmtInfo *fmt_info, BitVec *bv);
void _write_Int(Str *o, FmtInfo *fmt_info, Int *value);

const char *_read_Str(const char *i, FmtInfo *fmt_info, Str *s);
const char *_read_u8(const char *i, FmtInfo *fmt_info, u8 *v);
const char *_read_u16(const char *i, FmtInfo *fmt_info, u16 *v);
const char *_read_u32(const char *i, FmtInfo *fmt_info, u32 *v);
const char *_read_u64(const char *i, FmtInfo *fmt_info, u64 *v);
const char *_read_i8(const char *i, FmtInfo *fmt_info, i8 *v);
const char *_read_i16(const char *i, FmtInfo *fmt_info, i16 *v);
const char *_read_i32(const char *i, FmtInfo *fmt_info, i32 *v);
const char *_read_i64(const char *i, FmtInfo *fmt_info, i64 *v);
const char *_read_Zstr(const char *i, FmtInfo *fmt_info, const char **v);
const char *_read_UnsupportedType(const char *i, FmtInfo *fmt_info, const char **s);
const char *_read_f32(const char *i, FmtInfo *fmt_info, f32 *v);
const char *_read_f64(const char *i, FmtInfo *fmt_info, f64 *v);
const char *_read_Float(const char *i, FmtInfo *fmt_info, Float *value);
const char *_read_BitVec(const char *i, FmtInfo *fmt_info, BitVec *bv);
const char *_read_Int(const char *i, FmtInfo *fmt_info, Int *value);

#endif // MISRA_STD_IO
