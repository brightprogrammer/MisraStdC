/// file      : std/io.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// formatted reading/writing and other magical stuff

#ifndef MISRA_STD_IO_H
#define MISRA_STD_IO_H

#include <Misra/Std/Container.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/File.h>
#include <Misra/Types.h>

#include "Io/Private.h"

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
} Alignment, Endianness;

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
/// FMT_FLAG_ZERO_PAD      : Pad numeric output with '0' instead of ' ', drop base prefix.
///                          Triggered by a leading '0' before the width digits, e.g. {016x}
///                          renders u64 0xabc as "0000000000000abc". Sign (for signed
///                          integers) precedes the zero-fill: "{08}" of -42 -> "-0000042".
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
    FMT_FLAG_ZERO_PAD      = 1 << 10,
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
        Alignment  align;
        Endianness endian;
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
typedef bool (*TypeSpecificWriter)(Str *o, FmtInfo *fmt_info, void *data);

///
/// Unified I/O operations container
///
/// TAGS: I/O, Generic, Container
///
typedef Zstr (*TypeSpecificReader)(Zstr i, FmtInfo *fmt_info, void *data);

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

///
/// Explicit raw C-string I/O descriptor with allocator provenance.
///
/// This is used when a formatted I/O call needs to read into or rewrite a
/// caller-owned zero-terminated string pointer whose storage may already be
/// managed by a specific allocator.
///
/// value[in,out] : Address of the `char *` / `Zstr ` variable.
/// allocator[in] : Allocator responsible for any existing pointed-to storage.
///
/// TAGS: I/O, String, Allocator, Provenance
///
typedef struct {
    void      *value;
    Allocator *allocator;
} ZstrIOArg;

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

///
/// Create a `TypeSpecificIO` wrapper for raw C-string storage with explicit
/// allocator provenance.
///
/// This is primarily needed for formatted reads when the destination pointer
/// may already own memory that must be freed or replaced through a specific
/// allocator.
///
/// zstr[in,out]    : `char *` or `Zstr ` variable.
/// alloc_ptr[in]   : Allocator responsible for the pointed-to storage.
///
/// SUCCESS: Returns a `TypeSpecificIO` wrapper suitable for `StrReadFmt(...)`
///          or `FReadFmt(...)`.
/// FAILURE: Function cannot fail.
///
/// USAGE:
///   char *name = NULL;
///   Allocator alloc = DefaultAllocator();
///   StrReadFmt(text, "{s}", ZstrIO(name, &alloc));
///
/// TAGS: I/O, String, Allocator, Macro
///
#ifdef __cplusplus
#    define ZstrIO(zstr, alloc_ptr)                                                                                    \
        TO_TYPE_SPECIFIC_IO(ZstrAlloc, &LVAL(((ZstrIOArg) {.value = (void *)&(zstr), .allocator = (alloc_ptr)})))
#else
#    define ZstrIO(zstr, alloc_ptr)                                                                                    \
        ((TypeSpecificIO) {                                                                                            \
            .writer = (TypeSpecificWriter)_write_ZstrAlloc,                                                            \
            .reader = (TypeSpecificReader)_read_ZstrAlloc,                                                             \
            .data   = &((ZstrIOArg) {.value = (void *)&(zstr), .allocator = (alloc_ptr)}),                             \
        })
#endif

// `_Generic` case for each feature-gated bigint/bitvec type. When the
// feature is disabled, the case expands to nothing so the type name
// (which would otherwise have to be visible) is never mentioned.
#if FEATURE_BITVEC
#    define IOFMT_BITVEC_CASE_(x, addr)                                                                                \
BitVec:                                                                                                                \
        TO_TYPE_SPECIFIC_IO(BitVec, addr),
#else
#    define IOFMT_BITVEC_CASE_(x, addr)
#endif

#if FEATURE_INT
#    define IOFMT_INT_CASE_(x, addr)                                                                                   \
Int:                                                                                                                   \
        TO_TYPE_SPECIFIC_IO(Int, addr),
#else
#    define IOFMT_INT_CASE_(x, addr)
#endif

#if FEATURE_FLOAT
#    define IOFMT_FLOAT_CASE_(x, addr)                                                                                 \
Float:                                                                                                                 \
        TO_TYPE_SPECIFIC_IO(Float, addr),
#else
#    define IOFMT_FLOAT_CASE_(x, addr)
#endif

// The `default:` arm below is a runtime sentinel, not a silent cast:
// `_write_UnsupportedType` / `_read_UnsupportedType` LOG_FATAL on
// dispatch so a wrong-typed argument surfaces immediately instead of
// being coerced into one of the known writers.
#if defined(_MSC_VER) || defined(__MSC_VER)
#    define IOFMT(x)                                                                                                   \
        _Generic(                                                                                                      \
            (x),                                                                                                       \
            TypeSpecificIO: (x),                                                                                       \
            Str: TO_TYPE_SPECIFIC_IO(Str, &(x)),                                                                       \
            IOFMT_FLOAT_CASE_(x, &(x)) IOFMT_INT_CASE_(x, &(x)) IOFMT_BITVEC_CASE_(x, &(x))                            \
                Zstr : TO_TYPE_SPECIFIC_IO(Zstr, &(x)),                                                         \
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
// The `default:` arm below is a runtime sentinel, not a silent cast:
// `_write_UnsupportedType` / `_read_UnsupportedType` LOG_FATAL on
// dispatch so a wrong-typed argument surfaces immediately instead of
// being coerced into one of the known writers.
#    define IOFMT(x)                                                                                                   \
        _Generic(                                                                                                      \
            (x),                                                                                                       \
            TypeSpecificIO: (x),                                                                                       \
            Str: TO_TYPE_SPECIFIC_IO(Str, (void *)&(x)),                                                               \
            IOFMT_FLOAT_CASE_(x, (void *)&(x)) IOFMT_INT_CASE_(x, (void *)&(x)) IOFMT_BITVEC_CASE_(x, (void *)&(x))    \
                Zstr : TO_TYPE_SPECIFIC_IO(Zstr, (void *)&(x)),                                                 \
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

// Internal runtime functions backing the formatted-I/O macros live in
// `Misra/Std/Io/Private.h`. The umbrella `Misra/Std/Io.h` pulls them in via
// the `#include "Io/Private.h"` at the top of this header so macros can
// dispatch to them without exposing internal entry points as public API.

///
/// Pad `o` with ASCII spaces so its total length reaches `width`.
/// `content_len` is the length already considered "content" (typically
/// `StrLen(o)` at call time, or the length the caller plans to fill in
/// afterwards). Padding lands on the left, right, or both sides
/// depending on `align`. If `content_len >= width` the call is a no-op.
///
/// o[in,out]      : Str to pad. Grows through its inline allocator.
/// width[in]      : Target minimum width.
/// align[in]      : `ALIGN_LEFT` / `ALIGN_RIGHT` / `ALIGN_CENTER`.
/// content_len[in]: Current content length to anchor padding against.
///
/// SUCCESS : Returns `true`. `o` carries the requested padding.
/// FAILURE : Returns `false` on allocator OOM mid-pad. `o` may be left
///           partially padded.
///
/// TAGS: Str, Format, Pad
///
bool StrPad(Str *o, size width, Alignment align, size content_len);

#if FEATURE_FLOAT
///
/// Render `value` as a decimal-notation string into `*out`. When
/// `has_precision` is true, `precision` decimal digits are emitted
/// (padded with trailing zeros when needed); otherwise the canonical
/// form of the float is used (no truncation, no padding).
///
/// out[out]         : Receives a freshly-initialised `Str`. Caller
///                    `StrDeinit`s on either path.
/// value[in]        : Float to render.
/// precision[in]    : Number of fractional digits, used only when
///                    `has_precision` is true.
/// has_precision[in]: Whether `precision` should be applied.
/// alloc[in]        : Allocator backing `*out`.
///
/// SUCCESS : Returns `true`. `*out` carries the decimal rendering.
/// FAILURE : Returns `false` on allocator OOM. `*out` is left zeroed.
///
/// TAGS: Float, Format, Decimal
///
bool float_try_to_decimal_str(Str *out, Float *value, u32 precision, bool has_precision, Allocator *alloc);
#    define FloatTryToDecimalStr(...) MISRA_OVERLOAD(FloatTryToDecimalStr, __VA_ARGS__)
#    define FloatTryToDecimalStr_4(out, value, precision, has_precision)                                               \
        float_try_to_decimal_str((out), (value), (precision), (has_precision), MisraScope)
#    define FloatTryToDecimalStr_5(out, value, precision, has_precision, alloc)                                        \
        float_try_to_decimal_str((out), (value), (precision), (has_precision), ALLOCATOR_OF(alloc))

///
/// Render `value` as a scientific-notation string into `*out`
/// (`mantissa[.fraction]e[+-]exponent`). `uppercase` chooses `E` vs `e`.
///
/// out[out]         : Receives a freshly-initialised `Str`. Caller
///                    `StrDeinit`s on either path.
/// value[in]        : Float to render.
/// precision[in]    : Number of digits after the leading mantissa digit,
///                    used only when `has_precision` is true.
/// has_precision[in]: Whether `precision` should be applied.
/// uppercase[in]    : `true` -> `E+NN`, `false` -> `e+NN`.
/// alloc[in]        : Allocator backing `*out`.
///
/// SUCCESS : Returns `true`. `*out` carries the scientific rendering.
/// FAILURE : Returns `false` on allocator OOM. `*out` is left zeroed.
///
/// TAGS: Float, Format, Scientific
///
bool float_try_to_scientific_str(
    Str       *out,
    Float     *value,
    u32        precision,
    bool       has_precision,
    bool       uppercase,
    Allocator *alloc
);
#    define FloatTryToScientificStr(...) MISRA_OVERLOAD(FloatTryToScientificStr, __VA_ARGS__)
#    define FloatTryToScientificStr_5(out, value, precision, has_precision, uppercase)                                 \
        float_try_to_scientific_str((out), (value), (precision), (has_precision), (uppercase), MisraScope)
#    define FloatTryToScientificStr_6(out, value, precision, has_precision, uppercase, alloc)                          \
        float_try_to_scientific_str((out), (value), (precision), (has_precision), (uppercase), ALLOCATOR_OF(alloc))
#endif // FEATURE_FLOAT

///
/// Helper macro to append a comma after wrapping given argument in IOFMT
/// Used in following macros
///
#define IOFMT_LVAL_APPEND_COMMA(x) IOFMT(LVAL(x)),
#define IOFMT_APPEND_COMMA(x)      IOFMT(x),

///
/// Append a formatted string to `out`. Existing contents are preserved;
/// new content lands at the end of the buffer.
///
/// out[out]    : Destination `Str`. Existing bytes are kept.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variables to substitute. Wrap literals with `LVAL(...)`.
///
/// SUCCESS : `out` extended with the formatted content; returns `true`.
/// FAILURE : Returns `false`. May log diagnostics; `out` may be left
///           partially extended on allocation failure mid-write.
///
/// TAGS: Str, Append, Format, I/O
///
#define StrAppendFmt(out, ...) StrAppendFmt_IMPL1(out, __VA_ARGS__)
#define StrAppendFmt_IMPL1(input, fmtstr, ...)                                                                         \
    StrAppendFmt_IMPL2(                                                                                                \
        input,                                                                                                         \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define StrAppendFmt_IMPL2(input, fmtstr, varr)                                                                        \
    str_append_fmt((input), (fmtstr), &(varr)[0], (sizeof(varr) / sizeof(TypeSpecificIO)) - 1)

///
/// Write a formatted string to `out` from scratch. Equivalent to
/// `StrClear(out)` followed by `StrAppendFmt(out, ...)` -- any prior
/// contents of `out` are discarded.
///
/// TAGS: Str, Write, Format, I/O
///
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
    str_write_fmt((input), (fmtstr), &(varr)[0], (sizeof(varr) / sizeof(TypeSpecificIO)) - 1)

///
/// Patch existing bytes in `out` starting at `offset`. The formatted
/// content must fit within the current `out->length`; the buffer is
/// not grown. Useful for back-patching placeholder fields after later
/// data has been computed.
///
/// SUCCESS : Bytes `[offset, offset + written)` of `out` are replaced;
///           returns `true`.
/// FAILURE : Returns `false` if the formatted output would extend past
///           `out->length`. `out` is left unchanged.
///
/// TAGS: Str, Patch, Format, I/O
///
#define StrPatchFmt(out, offset, ...) StrPatchFmt_IMPL1(out, offset, __VA_ARGS__)
#define StrPatchFmt_IMPL1(input, offset, fmtstr, ...)                                                                  \
    StrPatchFmt_IMPL2(                                                                                                 \
        input,                                                                                                         \
        offset,                                                                                                        \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define StrPatchFmt_IMPL2(input, offset, fmtstr, varr)                                                                 \
    str_patch_fmt((input), (offset), (fmtstr), &(varr)[0], (sizeof(varr) / sizeof(TypeSpecificIO)) - 1)

///
/// Parse input string according to format string with rust-style placeholders,
/// extracting values into provided arguments. This is a macro wrapper around str_read_fmt.
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
/// FAILURE : Failure occurs within `str_read_fmt`. Refer to its documentation
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
#define StrReadFmt_IMPL2(input, fmtstr, varr)                                                                           \
    do {                                                                                                                \
        TypeSpecificIO *UNPL(argv)    = &(varr)[0];                                                                     \
        char          **UNPL(p_input) = (char **)(&(input));                                                            \
        u64             UNPL(argc)    = sizeof(varr) / sizeof(TypeSpecificIO);                                          \
        const char     *UNPL(out) = str_read_fmt((Zstr)*(UNPL(p_input)), (fmtstr), UNPL(argv), UNPL(argc) - 1); \
        (*UNPL(p_input))          = (char *)(UNPL(out)) ? (char *)(UNPL(out)) : (*UNPL(p_input));                       \
    } while (0)

///
/// Read formatted data from a file stream. This is a macro wrapper around f_read_fmt.
///
/// stream[in]  : Pointer to the `File` to read from.
/// fmtstr[in]  : Format string to be used for reading. This must exactly describe the
///               expected input format in the stream.
/// ...[in]     : Variable number of arguments that will receive the read values. Each
///               argument should be a modifiable l-value wrapped with `&variable`.
///
/// SUCCESS : Attempts to match `fmtstr` with the stream of characters in `stream` and
///           reads values into the provided arguments wrapped with ``.
/// FAILURE : Failure occurs within `f_read_fmt`. Refer to its documentation for
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
        TypeSpecificIO *UNPL(argv) = &(varr)[0];                                                                       \
        u64             UNPL(argc) = sizeof(varr) / sizeof(TypeSpecificIO) - 1;                                        \
        f_read_fmt((file), (fmtstr), UNPL(argv), UNPL(argc));                                                          \
    } while (0)

///
/// Write formatted output to a file stream. This macro internally uses str_write_fmt
/// to format the string and then writes it to the stream.
///
/// stream[in]  : Pointer to the `File` to write to.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `variable`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string is written to the specified `stream`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to the stream. Errors from `str_write_fmt`
///           (logging messages) might also occur.
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
    f_write_fmt((stream), (fmtstr), &(varr)[0], (sizeof(varr) / sizeof(TypeSpecificIO)) - 1, false)

///
/// Write formatted output to a file stream followed by a newline character.
/// This macro internally uses str_write_fmt to format the string and then writes
/// it to the stream followed by a newline.
///
/// stream[in]  : Pointer to the `File` to write to.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `variable`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string followed by a newline is written to the `stream`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to the stream. Errors from `str_write_fmt`
///           (logging messages) might also occur.
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
    f_write_fmt((stream), (fmtstr), &(varr)[0], (sizeof(varr) / sizeof(TypeSpecificIO)) - 1, true)

///
/// Write formatted output to the standard output stream (`FileStdout()`).
/// This is a convenience macro calling FWriteFmt with `FileStdout()`.
///
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `variable`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string is written to standard output.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation. Errors from `str_write_fmt` (logging
///           messages) might also occur.
///
/// TAGS: Macro, Convenience, Stdout, I/O
///
#define WriteFmt(...)                                                                                                  \
    do {                                                                                                               \
        File UNPL(out) = FileStdout();                                                                                 \
        FWriteFmt(&UNPL(out), __VA_ARGS__);                                                                            \
    } while (0)

///
/// Write formatted output to the standard output stream (`FileStdout()`) followed by a newline.
/// This is a convenience macro calling FWriteFmtLn with `FileStdout()`.
///
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to replace the placeholders. Each argument
///               should be wrapped with `variable`.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string followed by a newline is written to standard output.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation. Errors from `str_write_fmt` (logging
///           messages) might also occur.
///
/// TAGS: Macro, Convenience, Stdout, I/O
///
#define WriteFmtLn(...)                                                                                                \
    do {                                                                                                               \
        File UNPL(out) = FileStdout();                                                                                 \
        FWriteFmtLn(&UNPL(out), __VA_ARGS__);                                                                          \
    } while (0)

///
/// Read formatted input from the standard input stream (`FileStdin()`).
/// This is a convenience macro calling FReadFmt with `FileStdin()`.
///
/// fmtstr[in]  : Format string to be used for reading. This must exactly describe the
///               expected input format from standard input.
/// ...[in]     : Variable number of arguments that will receive the read values. Each
///               argument should be a modifiable l-value wrapped with `&variable`.
///
/// SUCCESS : Attempts to match `fmtstr` with the input from standard input and reads
///           values into the provided arguments wrapped with ``.
/// FAILURE : Failure occurs within `f_read_fmt`. Refer to its documentation for
///           details on failure behavior (logs error message and returns, may rollback
///           read data, or abort in unexpected situations).
///
/// TAGS: Macro, Convenience, Stdin, I/O
///
#define ReadFmt(...)                                                                                                   \
    do {                                                                                                               \
        File UNPL(in) = FileStdin();                                                                                   \
        FReadFmt(&UNPL(in), __VA_ARGS__);                                                                              \
    } while (0)

// not for direct use
bool _write_Str(Str *o, FmtInfo *fmt_info, Str *s);
bool _write_u8(Str *o, FmtInfo *fmt_info, u8 *v);
bool _write_u16(Str *o, FmtInfo *fmt_info, u16 *v);
bool _write_u32(Str *o, FmtInfo *fmt_info, u32 *v);
bool _write_u64(Str *o, FmtInfo *fmt_info, u64 *v);
bool _write_i8(Str *o, FmtInfo *fmt_info, i8 *v);
bool _write_i16(Str *o, FmtInfo *fmt_info, i16 *v);
bool _write_i32(Str *o, FmtInfo *fmt_info, i32 *v);
bool _write_i64(Str *o, FmtInfo *fmt_info, i64 *v);
bool _write_Zstr(Str *o, FmtInfo *fmt_info, Zstr *s);
bool _write_ZstrAlloc(Str *o, FmtInfo *fmt_info, ZstrIOArg *arg);
bool _write_UnsupportedType(Str *o, FmtInfo *fmt_info, Zstr *s);
bool _write_f32(Str *o, FmtInfo *fmt_info, f32 *v);
bool _write_f64(Str *o, FmtInfo *fmt_info, f64 *v);
#if FEATURE_FLOAT
bool _write_Float(Str *o, FmtInfo *fmt_info, Float *value);
#endif
#if FEATURE_BITVEC
bool _write_BitVec(Str *o, FmtInfo *fmt_info, BitVec *bv);
#endif
#if FEATURE_INT
bool _write_Int(Str *o, FmtInfo *fmt_info, Int *value);
#endif

Zstr _read_Str(Zstr i, FmtInfo *fmt_info, Str *s);
Zstr _read_u8(Zstr i, FmtInfo *fmt_info, u8 *v);
Zstr _read_u16(Zstr i, FmtInfo *fmt_info, u16 *v);
Zstr _read_u32(Zstr i, FmtInfo *fmt_info, u32 *v);
Zstr _read_u64(Zstr i, FmtInfo *fmt_info, u64 *v);
Zstr _read_i8(Zstr i, FmtInfo *fmt_info, i8 *v);
Zstr _read_i16(Zstr i, FmtInfo *fmt_info, i16 *v);
Zstr _read_i32(Zstr i, FmtInfo *fmt_info, i32 *v);
Zstr _read_i64(Zstr i, FmtInfo *fmt_info, i64 *v);
Zstr _read_Zstr(Zstr i, FmtInfo *fmt_info, Zstr *v);
Zstr _read_ZstrAlloc(Zstr i, FmtInfo *fmt_info, ZstrIOArg *arg);
Zstr _read_UnsupportedType(Zstr i, FmtInfo *fmt_info, Zstr *s);
Zstr _read_f32(Zstr i, FmtInfo *fmt_info, f32 *v);
Zstr _read_f64(Zstr i, FmtInfo *fmt_info, f64 *v);
#if FEATURE_FLOAT
Zstr _read_Float(Zstr i, FmtInfo *fmt_info, Float *value);
#endif
#if FEATURE_BITVEC
Zstr _read_BitVec(Zstr i, FmtInfo *fmt_info, BitVec *bv);
#endif
#if FEATURE_INT
Zstr _read_Int(Zstr i, FmtInfo *fmt_info, Int *value);
#endif

// ---------------------------------------------------------------------------
// Formatted Buf I/O. Moved out of Container/Buf.h because formatting is
// an I/O concern (Buf is a plain byte container; the FMT layer is built
// on TypeSpecificIO + FmtInfo which live here). Keeping these here also
// avoids a Buf.h -> Io.h -> File.h include cycle.
//
// `BufReadFmt(it, ...)` consumes raw bytes from a cursor.
// `BufAppendFmt(buf, ...)` adds raw bytes to the end of a Buf.
// `BufWriteFmt(buf, ...)` clears the Buf first, then appends.
// `BufPatchFmt(buf, offset, ...)` overwrites existing bytes at offset.
//
// Format string accepts only `{<Nr}` (LE) and `{>Nr}` (BE) directives,
// where N is 1, 2, 4, or 8. The destination variable's natural width
// must match the spec width.
// ---------------------------------------------------------------------------

bool buf_read_fmt(BufIter *iter, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);
bool buf_append_fmt(Buf *out, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);
bool buf_write_fmt(Buf *out, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);
bool buf_patch_fmt(Buf *out, size offset, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);

#define BufReadFmt(iter, ...) BufReadFmt_IMPL1((iter), __VA_ARGS__)
#define BufReadFmt_IMPL1(iter, fmtstr, ...)                                                                            \
    BufReadFmt_IMPL2(                                                                                                  \
        (iter),                                                                                                        \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                                    \
    })                                                                                                             \
    )
#define BufReadFmt_IMPL2(iter, fmtstr, varr)                                                                           \
    buf_read_fmt((iter), (fmtstr), &(varr)[0], sizeof(varr) / sizeof(TypeSpecificIO) - 1)

#define BufAppendFmt(buf, ...) BufAppendFmt_IMPL1((buf), __VA_ARGS__)
#define BufAppendFmt_IMPL1(buf, fmtstr, ...)                                                                           \
    BufAppendFmt_IMPL2(                                                                                                \
        (buf),                                                                                                         \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define BufAppendFmt_IMPL2(buf, fmtstr, varr)                                                                          \
    buf_append_fmt((buf), (fmtstr), &(varr)[0], sizeof(varr) / sizeof(TypeSpecificIO) - 1)

#define BufWriteFmt(buf, ...) BufWriteFmt_IMPL1((buf), __VA_ARGS__)
#define BufWriteFmt_IMPL1(buf, fmtstr, ...)                                                                            \
    BufWriteFmt_IMPL2(                                                                                                 \
        (buf),                                                                                                         \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define BufWriteFmt_IMPL2(buf, fmtstr, varr)                                                                           \
    buf_write_fmt((buf), (fmtstr), &(varr)[0], sizeof(varr) / sizeof(TypeSpecificIO) - 1)

#define BufPatchFmt(buf, offset, ...) BufPatchFmt_IMPL1((buf), (offset), __VA_ARGS__)
#define BufPatchFmt_IMPL1(buf, offset, fmtstr, ...)                                                                    \
    BufPatchFmt_IMPL2(                                                                                                 \
        (buf),                                                                                                         \
        (offset),                                                                                                      \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define BufPatchFmt_IMPL2(buf, offset, fmtstr, varr)                                                                   \
    buf_patch_fmt((buf), (offset), (fmtstr), &(varr)[0], sizeof(varr) / sizeof(TypeSpecificIO) - 1)

#endif // MISRA_STD_IO_H
