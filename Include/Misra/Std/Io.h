/// file      : std/io.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Public surface of the formatted-text I/O subsystem: the `{...}`
/// brace-language family (`StrAppendFmt`, `WriteFmt`, `FReadFmt`,
/// `ReadFmt`) and the binary-layout `BufReadFmt` / `BufAppendFmt` /
/// `BufWriteFmt` / `BufPatchFmt` family. Type-specific reader / writer
/// declarations sit at the bottom of the header so the public macros
/// can dispatch to them via `_Generic`.

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
/// Type-specific read callback signature.
///
/// TAGS: I/O, Callback, Generic
///
typedef Zstr (*TypeSpecificReader)(Zstr i, FmtInfo *fmt_info, void *data);

///
/// Unified I/O operations container. Bundles a writer + reader pair
/// over a single backing `data` pointer; one instance per type the
/// I/O machinery dispatches on. Produced via `TO_TYPE_SPECIFIC_IO(T, d)`
/// and consumed by the per-arg dispatch in the formatted-I/O macros.
///
/// FIELDS:
/// - writer : Type-specific write callback, or NULL when not used.
/// - reader : Type-specific read callback, or NULL when not used.
/// - data   : Pointer to the value being read/written.
///
/// TAGS: I/O, Generic, Container
///
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

///
/// Build a `TypeSpecificIO` value for type `T` over data pointer `d`.
/// Token-pastes `_write_T` and `_read_T` to pick the right callback
/// pair from the per-type writer/reader symbols.
///
/// T[in] : Type identifier (must match `_write_T` / `_read_T` symbols).
/// d[in] : Pointer to the value being read/written.
///
/// SUCCESS: Returns an initialised `TypeSpecificIO`.
/// FAILURE: Compile-time error if `_write_T` / `_read_T` are
///          undeclared for the given `T`.
///
/// TAGS: I/O, Macro, TypeConversion, Generic
///
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
/// zstr[in,out]    : `Zstr` variable.
/// alloc_ptr[in]   : Allocator responsible for the pointed-to storage.
///
/// SUCCESS: Returns a `TypeSpecificIO` wrapper suitable for `StrReadFmt(...)`
///          or `FReadFmt(...)`.
/// FAILURE: Function cannot fail.
///
/// USAGE:
///   Zstr name = NULL;
///   DefaultAllocator alloc = DefaultAllocatorInit();
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

/// Out-of-tree extension hook for `IOFMT(x)`. Define this macro BEFORE
/// including `Misra/Std/Io.h` (directly or transitively) in any TU that
/// wants `WriteFmt(..., user_value)` to dispatch through user-supplied
/// readers/writers. The macro must expand to a comma-terminated list of
/// `_Generic` arms naming the user type and binding it via
/// `TO_TYPE_SPECIFIC_IO(T, addr)`.
///
/// EXAMPLE (per-TU or per-project header):
///   #define IOFMT_USER_CASE_(x, addr)
///       MyWidget : TO_TYPE_SPECIFIC_IO(MyWidget, addr),
///       OtherT   : TO_TYPE_SPECIFIC_IO(OtherT,   addr),
///   #include <Misra/Std/Io.h>
/// (real definition needs trailing backslashes per macro syntax;
/// omitted here so this comment stays single-logical-line.)
///
/// The user must also provide `_write_<T>` / `_read_<T>` symbols with
/// signatures matching `TypeSpecificWriter` / `TypeSpecificReader`.
///
/// See `Docs/.../extending-io-with-user-types.md` for the full guide,
/// including the multi-library chain-extension pattern.
///
/// SUCCESS: Once defined before `<Misra/Std/Io.h>` is processed in a TU,
///          every listed type becomes an explicit `_Generic` arm in
///          `IOFMT(x)` for that TU; subsequent `WriteFmt` / `StrReadFmt`
///          / `FReadFmt` / `BufReadFmt` calls dispatch user values
///          through the matching `_write_T` / `_read_T` symbols with
///          the same compile-time selection as in-tree types.
/// FAILURE: Cannot fail at the macro-expansion layer. Mis-typed arms or
///          missing `_write_T` / `_read_T` symbols surface as compile or
///          link errors at the call site; a user value whose type is
///          not listed produces the standard `_Generic`
///          "no matching association" diagnostic.
///
/// TAGS: I/O, Generic, Extension, Macro
#ifndef IOFMT_USER_CASE_
#    define IOFMT_USER_CASE_(x, addr) /* empty -- override before include */
#endif

///
/// Type-aware format specifier generator.
///
/// Wrong types fail at compile time per the conventions' `_Generic`
/// dispatch rule (no `default:` arm). If the compiler reports
/// "controlling expression type not compatible with any generic
/// association" at an `IOFMT(x)` site, add an arm for the new type
/// (or convert `x` to one of the listed arm types before the call).
///
/// x[in] : Value to format
///
/// SUCCESS: Returns `TypeSpecificIO` for supported types.
/// FAILURE: Compile-time error on unsupported types.
///
/// TAGS: Macro, TypeDispatch, Generic, I/O, Format
///
#if defined(_MSC_VER) || defined(__MSC_VER)
#    define IOFMT(x)                                                                                                   \
        _Generic(                                                                                                      \
            (x),                                                                                                       \
            TypeSpecificIO: (x),                                                                                       \
            Str: TO_TYPE_SPECIFIC_IO(Str, &(x)),                                                                       \
            Buf: TO_TYPE_SPECIFIC_IO(Buf, &(x)),                                                                       \
            IOFMT_FLOAT_CASE_(x, &(x)) IOFMT_INT_CASE_(x, &(x)) IOFMT_BITVEC_CASE_(x, &(x)) IOFMT_USER_CASE_(x, &(x))  \
                Zstr: TO_TYPE_SPECIFIC_IO(Zstr, &(x)),                                                                 \
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
            char: TO_TYPE_SPECIFIC_IO(i8, &(x))                                                                        \
        )
#else
#    define IOFMT(x)                                                                                                   \
        _Generic(                                                                                                      \
            (x),                                                                                                       \
            TypeSpecificIO: (x),                                                                                       \
            Str: TO_TYPE_SPECIFIC_IO(Str, (void *)&(x)),                                                               \
            Buf: TO_TYPE_SPECIFIC_IO(Buf, (void *)&(x)),                                                               \
            IOFMT_FLOAT_CASE_(x, (void *)&(x)) IOFMT_INT_CASE_(x, (void *)&(x)) IOFMT_BITVEC_CASE_(x, (void *)&(x))    \
                IOFMT_USER_CASE_(x, (void *)&(x)) Zstr: TO_TYPE_SPECIFIC_IO(Zstr, (void *)&(x)),                       \
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
            char: TO_TYPE_SPECIFIC_IO(i8, (void *)&(x))                                                                \
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
#    define FloatTryToDecimalStr(...) OVERLOAD(FloatTryToDecimalStr, __VA_ARGS__)
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
#    define FloatTryToScientificStr(...) OVERLOAD(FloatTryToScientificStr, __VA_ARGS__)
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
/// SUCCESS : Returns `true`; `out` holds exactly the rendered output.
/// FAILURE : Returns `false` if the underlying append fails (allocator
///           failure or invalid format string). `out` is cleared either
///           way before the append runs, so a failed call leaves `out`
///           with whatever bytes were appended before the failure.
///           `LOG_FATAL` if `out` or the format string is NULL.
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
/// content must fit within the current `StrLen(out)`; the buffer is
/// not grown. Useful for back-patching placeholder fields after later
/// data has been computed.
///
/// SUCCESS : Bytes `[offset, offset + written)` of `out` are replaced;
///           returns `true`.
/// FAILURE : Returns `false` if the formatted output would extend past
///           `StrLen(out)`. `out` is left unchanged.
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
/// extracting values into provided arguments. Delegates to the in-tree
/// formatted-read backend.
///
/// NOTE: Provided input string must be an assignable l-value. The macro automatically updates given
///       input string to new parse position after a successful parse. If parse fails, the input
///       pointer does not change.
///
/// WARN: The input must not be passed to any allocator's deallocate call after use. The pointer
///       value is updated on successful read; after the call it might be `(input) + 1` or
///       `(input + 1233493783847394)`, neither of which is a valid free target.
///
/// WARN: Not providing an assignable input (first parameter) will result in undefined behavior.
///       If you're lucky you'll get a segfault.
///
/// INFO: The new `input` value after a successful read will be in [`input`, `input + len(input)`]
///
/// USAGE:
///    void ParseInput(Zstr in, Zstr *out_rest, DefaultAllocator *alloc) {
///        Zstr p  = in;                       // local cursor; in is not mutated
///        i32  id = 0;
///        Str  name = StrInit(alloc);
///        StrReadFmt(p, "Person id = {} and name = {}", &id, &name);
///        // p now points one past the consumed text on success.
///        *out_rest = p;
///        StrDeinit(&name);
///    }
///
/// input[in]   : Input string to parse (must be null-terminated).
/// fmtstr[in]  : Format string with `{}` placeholders (must be null-terminated).
/// ...[in]     : Variable number of arguments that will receive the parsed values. Each
///               argument should be a modifiable l-value wrapped with `&variable`.
///
/// SUCCESS : Statement form -- `input` is advanced in place to point at
///           the first character past the consumed text.
/// FAILURE : Backend logs an error and returns without consuming
///           `input`; `input` is left pointing at its original position.
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
        TypeSpecificIO *UNPL(argv) = &(varr)[0];                                                                       \
        u64             UNPL(argc) = sizeof(varr) / sizeof(TypeSpecificIO);                                            \
        Zstr            UNPL(out)  = str_read_fmt((input), (fmtstr), UNPL(argv), UNPL(argc) - 1);                      \
        if (UNPL(out))                                                                                                 \
            (input) = UNPL(out);                                                                                       \
    } while (0)

///
/// Read formatted data from a file stream. Delegates to the in-tree
/// file formatted-read backend.
///
/// stream[in]  : Pointer to the `File` to read from.
/// fmtstr[in]  : Format string to be used for reading. This must exactly describe the
///               expected input format in the stream.
/// ...[in]     : Variable number of arguments that will receive the read values. Each
///               argument should be a modifiable l-value wrapped with `&variable`.
///
/// SUCCESS : Attempts to match `fmtstr` with the stream of characters in `stream` and
///           reads values into the provided arguments.
/// FAILURE : Failure occurs within the file formatted-read backend
///           (logs error message and returns, may roll back read data,
///           or abort in unexpected situations).
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
/// Write formatted output to a file stream. Formats the rendered
/// string via the in-tree formatted-write backend and emits the result
/// to the stream.
///
/// stream[in]  : Pointer to the `File` to write to.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to substitute. Each entry runs through the IOFMT type dispatch.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string is written to the specified `stream`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to the stream; the backend
///           may also log an error message.
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
/// Formats the rendered string via the in-tree formatted-write backend
/// and emits the result to the stream, then appends a newline.
///
/// stream[in]  : Pointer to the `File` to write to.
/// fmtstr[in]  : Format string with `{}` placeholders.
/// ...[in]     : Variable number of arguments to substitute. Each entry runs through the IOFMT type dispatch.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string followed by a newline is written to the `stream`.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation to the stream; the backend
///           may also log an error message.
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
/// ...[in]     : Variable number of arguments to substitute. Each entry runs through the IOFMT type dispatch.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string is written to standard output.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation; the backend may also log
///           an error message.
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
/// ...[in]     : Variable number of arguments to substitute. Each entry runs through the IOFMT type dispatch.
///
/// SUCCESS : Placeholders in `fmtstr` are replaced by the passed arguments, and the
///           resulting formatted string followed by a newline is written to standard output.
/// FAILURE : Failure might occur during memory allocation for the temporary string
///           or during the write operation; the backend may also log
///           an error message.
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
///           values into the provided arguments.
/// FAILURE : Backend logs an error and returns; may roll back partially-
///           read data, or abort in unexpected situations.
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
bool _write_Buf(Str *o, FmtInfo *fmt_info, Buf *b);
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
Zstr _read_Buf(Zstr i, FmtInfo *fmt_info, Buf *b);
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

///
/// Backend for `BufReadFmt`. Walks `fmtstr` -- restricted to `{<Nr}`
/// (LE) and `{>Nr}` (BE) directives with `N` in {1, 2, 4, 8} -- and
/// reads each matching field from the bytes pointed at by `iter`,
/// advancing `iter` past every successfully consumed field.
///
/// SUCCESS : Returns `true`; `iter` has advanced past the consumed
///           bytes and every output slot in `argv` is populated.
/// FAILURE : Returns `false` on format-parse error (malformed
///           directive, spec width that does not match the destination
///           argument's natural width) or on cursor overflow (the
///           remaining bytes pointed at by `iter` are shorter than the
///           directive demands). `iter` may have advanced partially;
///           output slots written before the failure retain the
///           partially decoded value.
///
/// TAGS: Buf, Read, Format, I/O
///
bool buf_read_fmt(BufIter *iter, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);

///
/// Backend for `BufAppendFmt`. Encodes each `{<Nr}` / `{>Nr}` directive
/// in `fmtstr` from the matching `argv` slot and appends the raw bytes
/// to the end of `*out`, growing `out` through its inline allocator as
/// needed. Existing contents are preserved.
///
/// SUCCESS : Returns `true`; `BufLength(out)` has grown by exactly the
///           number of bytes the directives encode.
/// FAILURE : Returns `false` on format-parse error (malformed
///           directive, width mismatch with the source argument) or on
///           grow-buffer failure during append. `*out` may be left
///           partially extended -- bytes written before the failure
///           are kept.
///
/// TAGS: Buf, Append, Format, I/O
///
bool buf_append_fmt(Buf *out, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);

///
/// Backend for `BufWriteFmt`. Equivalent to clearing `*out` and then
/// running the formatted-append path -- any prior contents of `out`
/// are discarded before encoding starts.
///
/// SUCCESS : Returns `true`; `*out` holds exactly the encoded bytes.
/// FAILURE : Returns `false` on format-parse error or grow-buffer
///           failure. `out` is cleared either way before the append
///           runs, so a failed call leaves `out` with whatever bytes
///           were encoded before the failure (and no leftover prior
///           content).
///
/// TAGS: Buf, Write, Format, I/O
///
bool buf_write_fmt(Buf *out, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);

///
/// Backend for `BufPatchFmt`. Overwrites bytes of `*out` starting at
/// `offset` with the encoded output. The buffer is NOT grown; the
/// directive run must fit inside the existing `[offset, BufLength(out))`
/// window. Useful for back-patching length / checksum fields after the
/// payload they describe has been computed.
///
/// SUCCESS : Returns `true`; bytes `[offset, offset + written)` of
///           `*out` are replaced.
/// FAILURE : Returns `false` on format-parse error or if the encoded
///           output would extend past `BufLength(out)`. On overflow,
///           `*out` is left unchanged.
///
/// TAGS: Buf, Patch, Format, I/O
///
bool buf_patch_fmt(Buf *out, size offset, Zstr fmtstr, TypeSpecificIO *argv, u64 argc);

///
/// Read raw bytes from the cursor `iter` according to `fmtstr`. Only
/// `{<Nr}` (LE) and `{>Nr}` (BE) directives with `N` in {1, 2, 4, 8}
/// are accepted; the destination variable's natural width must match
/// the spec width.
///
/// SUCCESS : Returns `true`; `iter` advances past the consumed bytes
///           and every destination variable is filled.
/// FAILURE : Returns `false` on format-parse error or cursor overflow.
///           `iter` and destination variables may be left partially
///           updated by directives that ran before the failure.
///
/// TAGS: Buf, Read, Format, I/O
///
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

///
/// Append encoded bytes to the end of `buf`. Existing contents are
/// preserved; new bytes land after `BufLength(buf)`. Directives are
/// `{<Nr}` / `{>Nr}` with `N` in {1, 2, 4, 8}.
///
/// SUCCESS : Returns `true`; `buf` extended by exactly the encoded
///           bytes.
/// FAILURE : Returns `false` on format-parse error or grow-buffer
///           failure. `buf` may be left partially extended.
///
/// TAGS: Buf, Append, Format, I/O
///
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

///
/// Write encoded bytes to `buf` from scratch. Equivalent to clearing
/// `buf` and then `BufAppendFmt(buf, ...)` -- any prior contents are
/// discarded.
///
/// SUCCESS : Returns `true`; `buf` holds exactly the encoded bytes.
/// FAILURE : Returns `false` on format-parse error or grow-buffer
///           failure. `buf` is cleared before the append runs, so a
///           failed call leaves `buf` with whatever bytes were encoded
///           before the failure.
///
/// TAGS: Buf, Write, Format, I/O
///
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

///
/// Overwrite existing bytes of `buf` starting at `offset`. The encoded
/// output must fit within the current `BufLength(buf)`; the buffer is not
/// grown. Useful for back-patching placeholder fields (lengths,
/// checksums) after the payload they describe has been built.
///
/// SUCCESS : Returns `true`; bytes `[offset, offset + written)` of
///           `buf` are replaced.
/// FAILURE : Returns `false` on format-parse error or if the encoded
///           output would extend past `BufLength(buf)`. On overflow,
///           `buf` is left unchanged.
///
/// TAGS: Buf, Patch, Format, I/O
///
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
