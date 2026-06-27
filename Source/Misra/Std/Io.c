/// file      : std/io.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Formatted text I/O. Backs the `WriteFmt` / `ReadFmt` /
/// `StrAppendFmt` / `BufReadFmt` family by walking a `{...}` brace
/// language over a typed-arg vector. The reader/writer entry-points
/// per type (`_read_u32` / `_write_Float` / ...) are declared in
/// `Io.h` so the public macros can name them through `_Generic`.

#include <Misra/Config.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys.h>
#include <Misra/Types.h>

#if FEATURE_BITVEC
#    include <Misra/Std/Container/BitVec.h>
#endif
#if FEATURE_INT
#    include <Misra/Std/Container/Int.h>
#endif
#if FEATURE_FLOAT
#    include <Misra/Std/Container/Float.h>
#endif

#include <Misra/Std/Math.h>

// In-tree hex helpers used by the escape-decoder and the BitVec hex
// readers; the library deliberately does not pull a hex-classification
// helper from the platform.
// Returns -1 on a non-hex digit, otherwise 0..15.
static inline i32 hex_nibble(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

// Parses two hex chars into a byte; returns -1 if either is not hex.
static inline i32 hex_byte(char hi, char lo) {
    i32 h = hex_nibble(hi);
    i32 l = hex_nibble(lo);
    if (h < 0 || l < 0)
        return -1;
    return (h << 4) | l;
}

static bool _write_r8(Str *o, FmtInfo *fmt_info, u8 *v);
static bool _write_r16(Str *o, FmtInfo *fmt_info, u16 *v);
static bool _write_r32(Str *o, FmtInfo *fmt_info, u32 *v);
static bool _write_r64(Str *o, FmtInfo *fmt_info, u64 *v);

static Zstr _read_r8(Zstr i, FmtInfo *fmt_info, u8 *v);
static Zstr _read_r16(Zstr i, FmtInfo *fmt_info, u16 *v);
static Zstr _read_r32(Zstr i, FmtInfo *fmt_info, u32 *v);
static Zstr _read_r64(Zstr i, FmtInfo *fmt_info, u64 *v);

// Avoids `__builtin_clzll`: we'd need a separate MSVC branch via
// `_BitScanReverse64`, and the float-formatter only calls this once
// per render, so the portable binary-search shape pays for itself.
static inline u64 count_leading_zeros_u64(u64 value) {
    if (value == 0)
        return 64;

    u64 count = 0;

    if ((value >> 32) == 0) {
        count  += 32;
        value <<= 32;
    }
    if ((value >> 48) == 0) {
        count  += 16;
        value <<= 16;
    }
    if ((value >> 56) == 0) {
        count  += 8;
        value <<= 8;
    }
    if ((value >> 60) == 0) {
        count  += 4;
        value <<= 4;
    }
    if ((value >> 62) == 0) {
        count  += 2;
        value <<= 2;
    }
    if ((value >> 63) == 0) {
        count += 1;
    }

    return count;
}

// Brace-body grammar: {[(<|>|^)][0][width][.precision][type]} where
// `<`/`>`/`^` double as align (text) and endianness (raw); a leading
// `0` before width switches numeric pad from space to '0' and drops
// any base prefix; type is one of `c`/`a`/`A`/`x`/`X`/`b`/`o`/`r`/`e`/
// `E`/`s`.
static bool parse_format_spec(Zstr spec, u32 len, FmtInfo *fi) {
    if (!spec || !fi) {
        LOG_FATAL("Invalid arguments");
        return false;
    }
    // `len == 0` (the empty `{}` body) is the well-formed default-spec
    // case and must fall through to the defaulted FmtInfo below.

    *fi = (FmtInfo) {.align = ALIGN_RIGHT, .width = 0, .precision = 6, .flags = FMT_FLAG_NONE};

    u32 pos = 0;

    if (len) {
        switch (spec[pos]) {
            case '<' :
                fi->align = ALIGN_LEFT; // or ENDIAN_SMALL
                pos++;
                break;
            case '>' :
                fi->align = ALIGN_RIGHT; // or ENDIAN_BIG
                pos++;
                break;
            case '^' :
                fi->align = ALIGN_CENTER; // or ENDIAN_NATIVE
                pos++;
                break;
            default :
                fi->align = ALIGN_RIGHT;
                break;
        }
    }

    // A leading '0' followed by at least one more digit means "pad
    // numeric output with '0' and drop any base prefix" (matches
    // C/Python {:0Nx} convention). `{0}` alone stays a zero-width spec.
    if (pos + 1 < len && spec[pos] == '0' && spec[pos + 1] >= '0' && spec[pos + 1] <= '9') {
        fi->flags |= FMT_FLAG_ZERO_PAD;
        pos++;
    }

    u32 width = 0;
    if (pos < len && spec[pos] >= '0' && spec[pos] <= '9') {
        while (pos < len && spec[pos] >= '0' && spec[pos] <= '9') {
            width = width * 10 + (spec[pos] - '0');
            pos++;
        }
    }
    fi->width = width;

    if (pos < len && spec[pos] == '.') {
        pos++;
        if (pos >= len || spec[pos] < '0' || spec[pos] > '9') {
            // A '.' without trailing digits is malformed.
            return false;
        }
        size precision = 0;
        while (pos < len && spec[pos] >= '0' && spec[pos] <= '9') {
            precision = precision * 10 + (spec[pos] - '0');
            pos++;
        }
        fi->flags     |= FMT_FLAG_HAS_PRECISION;
        fi->precision  = precision;
    }

    while (pos < len) {
        switch (spec[pos]) {
            case 'c' :
                fi->flags |= FMT_FLAG_CHAR;
                break;

            case 'A' :
                fi->flags |= FMT_FLAG_CAPS | FMT_FLAG_FORCE_CASE | FMT_FLAG_CHAR;
                break;
            case 'a' :
                fi->flags |= FMT_FLAG_FORCE_CASE | FMT_FLAG_CHAR;
                break;

            case 'X' :
                fi->flags |= FMT_FLAG_HEX | FMT_FLAG_CAPS;
                break;
            case 'x' :
                fi->flags |= FMT_FLAG_HEX;
                break;

            case 'b' :
                fi->flags |= FMT_FLAG_BINARY;
                break;
            case 'o' :
                fi->flags |= FMT_FLAG_OCTAL;
                break;

            case 'r' :
                fi->flags |= FMT_FLAG_RAW;
                break;

            case 'E' :
                fi->flags |= FMT_FLAG_SCIENTIFIC | FMT_FLAG_CAPS;
                break;
            case 'e' :
                fi->flags |= FMT_FLAG_SCIENTIFIC;
                break;

            case 's' :
                fi->flags |= FMT_FLAG_STRING;
                break;

            default :
                LOG_ERROR("Invalid format specifier");
                return false;
        }
        pos++;
    }

    if (pos < len) {
        LOG_ERROR(
            "Parsing format specifier ended, but more characters are left for parsing. Indicates invalid format "
            "specifier."
        );
        return false;
    }

    return true;
}

// Zero-fill a numeric value that was rendered into `o` starting at
// `content_start`. The fill goes between the sign (if any) and the
// first digit, producing e.g. "-0000042" rather than "0000-0042".
// width and content_len are measured in chars (sign counts as content).
static bool pad_numeric_zeros(Str *o, size content_start, size width, size content_len) {
    if (content_len >= width) {
        return true;
    }
    size pad_len = width - content_len;
    // Detect a leading sign character; if present, the '0' fill goes
    // after it. Otherwise insert at the start of the content.
    size insert_pos = content_start;
    if (content_len > 0 && (StrCharAt(o, content_start) == '-' || StrCharAt(o, content_start) == '+')) {
        insert_pos += 1;
    }
    for (size i = 0; i < pad_len; i++) {
        if (!StrInsertR(o, '0', insert_pos)) {
            return false;
        }
    }
    return true;
}

bool StrPad(Str *o, size width, Alignment align, size content_len) {
    if (content_len >= width)
        return true;

    size pad_len = width - content_len;

    if (align == ALIGN_RIGHT) {
        for (size i = 0; i < pad_len; i++) {
            if (!StrPushFrontR(o, ' ')) {
                return false;
            }
        }
    } else if (align == ALIGN_LEFT) {
        for (size i = 0; i < pad_len; i++) {
            if (!StrPushBackR(o, ' ')) {
                return false;
            }
        }
    } else { // ALIGN_CENTER
        size left_pad  = pad_len / 2;
        size right_pad = pad_len - left_pad;

        for (size i = 0; i < left_pad; i++) {
            if (!StrPushFrontR(o, ' ')) {
                return false;
            }
        }
        for (size i = 0; i < right_pad; i++) {
            if (!StrPushBackR(o, ' ')) {
                return false;
            }
        }
    }

    return true;
}

bool str_append_fmt(Str *o, Zstr fmt, TypeSpecificIO *args, u64 argc) {
    if (!o || !fmt) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    size arg_idx = 0;
    size fmt_len = 0;
    while (fmt[fmt_len])
        fmt_len++;

    for (size i = 0; i < fmt_len; i++) {
        if (fmt[i] == '{') {
            // `{{` is the escape for a literal '{'.
            if (i + 1 < fmt_len && fmt[i + 1] == '{') {
                if (!StrPushBackR(o, '{')) {
                    return false;
                }
                i++;
                continue;
            }

            size brace_start = i;
            size brace_end   = i + 1;
            while (brace_end < fmt_len && fmt[brace_end] != '}') {
                brace_end++;
            }
            if (brace_end >= fmt_len) {
                LOG_ERROR("Unclosed format specifier");
                return false;
            }

            size    spec_len = brace_end - brace_start - 1;
            FmtInfo fmt_info;
            if (spec_len == 0) {
                // Empty `{}` body falls through to the same defaults
                // parse_format_spec would have emitted for an empty
                // input, without the bounds check overhead.
                fmt_info = (FmtInfo) {.align = ALIGN_RIGHT, .width = 0, .precision = 6, .flags = FMT_FLAG_NONE};
            } else if (!parse_format_spec(fmt + brace_start + 1, spec_len, &fmt_info)) {
                return false;
            }

            if (arg_idx >= argc) {
                LOG_ERROR("Not enough arguments for format string");
                return false;
            }

            TypeSpecificIO *arg = &args[arg_idx++];
            if (!arg->writer || !arg->data) {
#if defined(_MSC_VER) || defined(__MSC_VER)
                // MSVC C `_Generic` types string literals as `char *`
                // regardless of `/Zc:strictStrings` (C++-only flag); the
                // IOFMT char-arm therefore can't pick a per-type writer
                // for literals. Fall back to a default writer.
                if (fmt_info.flags & FMT_FLAG_CHAR) {
                    arg->writer = (TypeSpecificWriter)_write_i8;
                } else {
                    arg->writer = (TypeSpecificWriter)_write_u64;
                }
#else
                LOG_FATAL("Invalid writer or data pointer");
#endif
            }

            // `{Nr}` bypasses the type-specific writer in favour of the
            // raw byte-pair pipeline below; identity-compare the writer
            // pointer to recover the source variable's natural width.
            if (fmt_info.flags & FMT_FLAG_RAW) {
                TypeSpecificWriter write_fn  = arg->writer;
                u32                var_width = 0;
                if (write_fn == (void *)_write_u8 || write_fn == (void *)_write_i8) {
                    var_width = 1;
                } else if (write_fn == (void *)_write_u16 || write_fn == (void *)_write_i16) {
                    var_width = 2;
                } else if (write_fn == (void *)_write_u32 || write_fn == (void *)_write_i32 ||
                           write_fn == (void *)_write_f32) {
                    var_width = 4;
                } else if (write_fn == (void *)_write_u64 || write_fn == (void *)_write_i64 ||
                           write_fn == (void *)_write_f64) {
                    var_width = 8;
                } else {
                    LOG_ERROR(
                        "Raw data writing can only be used for u8-64, i8-64, f32, f64. Either unsupported format or "
                        "attempt to write a complex type."
                    );
                    return false;
                }

                if (fmt_info.width > var_width) {
                    LOG_ERROR(
                        "Number of raw bytes to be written exceeds variable width. Excess data filled with zeroes."
                    );
                }

                // Widen-then-narrow keeps the byte pickup endian-clean
                // regardless of the source's natural width.
                u64 x = 0;
                switch (var_width) {
                    case 1 : {
                        x = *(u8 *)arg->data;
                        break;
                    }
                    case 2 : {
                        x = *(u16 *)arg->data;
                        break;
                    }
                    case 4 : {
                        x = *(u32 *)arg->data;
                        break;
                    }
                    case 8 : {
                        x = *(u64 *)arg->data;
                        break;
                    }
                    default : {
                        LOG_ERROR("Unreachable code reached");
                        return false;
                    }
                }

                switch (fmt_info.width) {
                    case 1 : {
                        u8 y = (u8)x;
                        if (!_write_u8(o, &fmt_info, &y)) {
                            return false;
                        }
                        break;
                    }
                    case 2 : {
                        u16 y = (u16)x;
                        if (!_write_u16(o, &fmt_info, &y)) {
                            return false;
                        }
                        break;
                    }
                    case 4 : {
                        u32 y = (u32)x;
                        if (!_write_u32(o, &fmt_info, &y)) {
                            return false;
                        }
                        break;
                    }
                    case 8 : {
                        if (!_write_u64(o, &fmt_info, &x)) {
                            return false;
                        }
                        break;
                    }
                    default : {
                        LOG_ERROR("Unreachable code reached");
                        return false;
                    }
                }
            } else {
                if (!arg->writer(o, &fmt_info, arg->data)) {
                    return false;
                }
            }

            i = brace_end;
        } else if (fmt[i] == '}') {
            // `}}` escapes to a single literal '}'.
            if (i + 1 < fmt_len && fmt[i + 1] == '}') {
                if (!StrPushBackR(o, '}')) {
                    return false;
                }
                i++;
                continue;
            }
            LOG_ERROR("Unmatched closing brace");
            return false;
        } else {
            if (!StrPushBackR(o, fmt[i])) {
                return false;
            }
        }
    }

    if (arg_idx < argc) {
        LOG_ERROR("Too many arguments for format string");
        return false;
    }

    return true;
}

bool str_write_fmt(Str *o, Zstr fmt, TypeSpecificIO *args, u64 argc) {
    if (!o || !fmt) {
        LOG_FATAL("Invalid arguments");
        return false;
    }
    StrClear(o);
    return str_append_fmt(o, fmt, args, argc);
}

bool str_patch_fmt(Str *o, size offset, Zstr fmt, TypeSpecificIO *args, u64 argc) {
    if (!o || !fmt) {
        LOG_FATAL("Invalid arguments");
        return false;
    }
    // Render into a scratch Str, then overwrite [offset, offset+rendered.length).
    // DefaultAllocator is the right fit -- the rendered output size depends on
    // the caller-supplied fmt string and arguments, so no stack-bound makes
    // sense. Library-internal scratch keeps the debug-build instrumentation.
    DefaultAllocator scratch = DefaultAllocatorInit();
    Str              tmp     = StrInit(&scratch);
    bool             ok      = str_append_fmt(&tmp, fmt, args, argc);
    if (ok) {
        if (offset > StrLen(o) || StrLen(&tmp) > StrLen(o) - offset) {
            LOG_ERROR(
                "StrPatchFmt: write of {} bytes at offset {} exceeds str length {}",
                StrLen(&tmp),
                offset,
                StrLen(o)
            );
            ok = false;
        } else if (StrLen(&tmp)) {
            MemCopy(StrBegin(o) + offset, StrBegin(&tmp), StrLen(&tmp));
        }
    }
    StrDeinit(&tmp);
    DefaultAllocatorDeinit(&scratch);
    return ok;
}

bool f_write_fmt(File *stream, Zstr fmtstr, TypeSpecificIO *argv, u64 argc, bool append_newline) {
    Str  out;
    bool ok = true;
    // DefaultAllocator: rendered line size is caller-controlled (depends on the
    // fmt string + args), so no fixed stack cap fits all callers.
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!stream || !fmtstr) {
        LOG_FATAL("Invalid arguments");
        DefaultAllocatorDeinit(&scratch);
        return false;
    }

    out = StrInit(&scratch);
    ok  = str_append_fmt(&out, fmtstr, argv, argc);

    // Build the whole line first, including any trailing newline, then
    // emit in a single FileWrite. Single-syscall writes under PIPE_BUF
    // (4096) are atomic on POSIX, so concurrent threads don't shred
    // each other's output.
    if (ok && append_newline) {
        StrPushBackR(&out, '\n');
    }

    if (ok && StrLen(&out) > 0 && FileWrite(stream, StrBegin(&out), StrLen(&out)) != (i64)StrLen(&out)) {
        LOG_ERROR("Failed to write formatted output");
        ok = false;
    }

    if (ok && !FileFlush(stream)) {
        LOG_ERROR("Failed to flush formatted output");
        ok = false;
    }

    StrDeinit(&out);
    DefaultAllocatorDeinit(&scratch);
    return ok;
}

Zstr str_read_fmt(Zstr input, Zstr fmtstr, TypeSpecificIO *argv, u64 argc) {
    if (!input || !fmtstr) {
        LOG_FATAL("Invalid arguments");
    }

    Zstr p         = fmtstr;
    Zstr in        = input;
    u64  rem_p     = ZstrLen(fmtstr);
    u64  rem_in    = ZstrLen(in);
    u64  arg_index = 0; // Current argument index

    while (rem_p > 0) {
        if (rem_p >= 2 && p[0] == '{' && p[1] == '{') {
            if (!in || *in != '{') {
                LOG_ERROR("Expected '{' in input");
                return NULL;
            }
            in++;
            p     += 2;
            rem_p -= 2;
        } else if (rem_p >= 2 && p[0] == '}' && p[1] == '}') {
            if (!in || *in != '}') {
                LOG_ERROR("Expected '}' in input");
                return NULL;
            }
            in++;
            p     += 2;
            rem_p -= 2;
        } else if (p[0] == '{') {
            p++;
            rem_p--;

            Zstr start    = p;
            size spec_len = 0;
            while (rem_p > 0 && *p != '}') {
                p++;
                rem_p--;
                spec_len++;
            }

            if (rem_p == 0 || *p != '}') {
                LOG_ERROR("Unmatched '{' in format string");
                return NULL;
            }

            // 32 chars is the StrInitStack scratch below; any spec
            // longer than that is malformed by construction.
            if (spec_len >= 32) {
                LOG_ERROR("Format specifier too long");
                return NULL;
            }

            if (arg_index >= argc) {
                LOG_ERROR("More placeholders than arguments");
                return NULL;
            }

            // Validate format specifier. The spec scratch lives only
            // long enough to NUL-terminate the slice for parse_format_spec.
            FmtInfo fmt_info = {0};
            bool    spec_ok  = false;
            StrInitStack(spec_buf, 32) {
                char *data = StrBegin(&spec_buf);
                MemCopy(data, start, spec_len);
                data[spec_len] = '\0';
                StrResize(&spec_buf, (size)spec_len);
                spec_ok = parse_format_spec(data, spec_len, &fmt_info);
            }
            if (!spec_ok) {
                LOG_ERROR("Failed to parse format specifier");
                return NULL;
            }
            fmt_info.max_read_len = rem_in;

            // Step past the closing '}' so the body below sees the
            // first byte after the specifier.
            p++;
            rem_p--;

            TypeSpecificIO *io = &argv[arg_index++];
            if (!io->reader) {
                LOG_ERROR("Missing reader function");
                return NULL;
            }

            // Raw-data directives dispatch to the byte-level reader
            // table instead of the type-specific reader the caller
            // registered.
            Zstr               next       = NULL;
            TypeSpecificReader raw_reader = NULL;
            if (fmt_info.flags & FMT_FLAG_RAW) {
                switch (fmt_info.width) {
                    case 1 : {
                        raw_reader = (TypeSpecificReader)_read_r8;
                        break;
                    }
                    case 2 : {
                        raw_reader = (TypeSpecificReader)_read_r16;
                        break;
                    }
                    case 4 : {
                        raw_reader = (TypeSpecificReader)_read_r32;
                        break;
                    }
                    case 8 : {
                        raw_reader = (TypeSpecificReader)_read_r64;
                        break;
                    }
                    default : {
                        LOG_ERROR("Invalid raw data read width specified. Must be one of 1, 2, 4 or 8.");
                        return NULL;
                    }
                }

                // Known limitation: _read_r{8,16,32,64} read
                // fmt_info.width bytes without bound checking, and the
                // surrounding rem_in here is derived from ZstrLen(in)
                // -- which is misleading for binary inputs that have
                // embedded zeros (the parser callers feed raw file
                // bytes where embedded zeros are common). Adding a
                // `rem_in < fmt_info.width` check here breaks the
                // binary parsers. Closing this properly requires
                // threading a real buffer length through str_read_fmt
                // instead of ZstrLen; tracked as a separate refactor.
                u64 x = 0;
                next  = raw_reader(in, &fmt_info, &x);

                if (next) {
                    rem_in -= (next - in);
                }

                // Identity-compare the reader pointer to recover the
                // destination variable's natural width, same trick the
                // raw-write path uses.
                u32   var_width = 0;
                void *read_fn   = (void *)io->reader;
                if (read_fn == (void *)_read_u8 || read_fn == (void *)_read_i8) {
                    var_width = 1;
                } else if (read_fn == (void *)_read_u16 || read_fn == (void *)_read_i16) {
                    var_width = 2;
                } else if (read_fn == (void *)_read_u32 || read_fn == (void *)_read_i32 ||
                           read_fn == (void *)_read_f32) {
                    var_width = 4;
                } else if (read_fn == (void *)_read_u64 || read_fn == (void *)_read_i64 ||
                           read_fn == (void *)_read_f64) {
                    var_width = 8;
                } else {
                    LOG_ERROR(
                        "Raw data reading can only be used for u8-64, i8-64, f32, f64. Either unsupported format or "
                        "attempt to read a complex type."
                    );
                    return NULL;
                }

                if (fmt_info.width > var_width) {
                    LOG_INFO("Number of bytes read as raw data exceeds variable width. Excess data will discarded.");
                }

                // Narrow-on-store keeps the upper bytes off the
                // destination when the spec read more than the variable
                // can hold.
                switch (var_width) {
                    case 1 : {
                        *(u8 *)io->data = (u8)x;
                        break;
                    }
                    case 2 : {
                        *(u16 *)io->data = (u16)x;
                        break;
                    }
                    case 4 : {
                        *(u32 *)io->data = (u32)x;
                        break;
                    }
                    case 8 : {
                        *(u64 *)io->data = x;
                        break;
                    }
                    default : {
                        LOG_ERROR("Invalid raw data read width specified. Must be one of 1, 2, 4 or 8.");
                        return NULL;
                    }
                }
            } else {
                // Cap the field by the literal run between this
                // placeholder and the next `{`. The literal anchors
                // where the field ends; without it, greedy readers
                // would consume past the boundary the format author
                // intended.
                u64  space_len = 0;
                char c         = p[space_len];
                while (c) {
                    if (c == '{' && p[space_len + 1] != '{') {
                        break;
                    } else {
                        space_len++;
                    }
                    c = p[space_len];
                }

                if (space_len) {
                    Zstr e = NULL;
                    if ((e = ZstrFindSubstringN(in, p, space_len))) {
                        fmt_info.max_read_len = e - in;
                    }
                } else {
                    fmt_info.max_read_len = rem_in;
                }

                next = io->reader(in, &fmt_info, io->data);

                if (next) {
                    rem_in -= (next - in);
                }
            }

            if (!next || next == in) {
                LOG_ERROR("Failed to read value for placeholder {}", LVAL(arg_index - 1));
                return NULL;
            }

            in = next;
        } else {
            if (!in || *in != *p) {
                LOG_ERROR(
                    "Input '{.8}' does not match format string '{.8}'",
                    LVAL(in ? in : "(null)"),
                    LVAL(p ? p : "(null)")
                );
                return NULL;
            }
            in++;
            p++;
            rem_p--;
            rem_in--;
        }
    }

    return in;
}

// ---------------------------------------------------------------------------
// buf_read_fmt / buf_append_fmt / buf_write_fmt / buf_patch_fmt:
// formatted binary I/O over a BufIter (read) or Buf (write/append/patch).
// All four share the same on-disk format vocabulary: `{<Nr}` / `{>Nr}` for
// raw N-byte LE/BE reads/writes where N is 1/2/4/8. The destination /
// source variable width must match the spec width exactly.
//
// Read is atomic: on any field failure iter->pos is restored to entry
// value. Append/Write fail-stop on first allocation failure, leaving the
// Buf in whatever state the partial run left it (caller may have to
// truncate). Patch validates fit before mutating, then writes in-place.
// ---------------------------------------------------------------------------

bool buf_read_fmt(BufIter *iter, Zstr fmtstr, TypeSpecificIO *argv, u64 argc) {
    if (!iter || !fmtstr) {
        LOG_FATAL("buf_read_fmt: NULL iter or fmtstr");
    }

    BufIter start     = *iter;
    u64     arg_index = 0;
    StrIter fsi       = StrIterFromZstr(fmtstr);
    char    fc        = 0;

    while (StrIterPeek(&fsi, &fc)) {
        if (fc != '{') {
            // Literal byte: must match the cursor exactly. A mismatch or
            // a short buffer is a soft parse failure (rewind + false),
            // not an abort -- this is how magic bytes are checked inline.
            if (IterRemainingLength(iter) < 1 || *(const u8 *)IterPos(iter) != (u8)fc) {
                *iter = start;
                return false;
            }
            IterMustMove(iter, 1);
            StrIterMustNext(&fsi);
            continue;
        }
        StrIterMustNext(&fsi); // step over '{'
        // `{{` escapes to a single literal '{' byte.
        char esc = 0;
        if (StrIterPeek(&fsi, &esc) && esc == '{') {
            if (IterRemainingLength(iter) < 1 || *(const u8 *)IterPos(iter) != (u8)'{') {
                *iter = start;
                return false;
            }
            IterMustMove(iter, 1);
            StrIterMustNext(&fsi); // step over second '{'
            continue;
        }
        StrIter spec_start_fsi = fsi;
        Zstr    spec_start     = (Zstr)StrIterPos(&fsi);
        char    sc             = 0;
        while (StrIterPeek(&fsi, &sc) && sc != '}') {
            StrIterMustNext(&fsi);
        }
        if (sc != '}') {
            LOG_FATAL("buf_read_fmt: unterminated {{ in fmt");
        }
        u32 spec_len = (u32)(StrIterRemainingLength(&spec_start_fsi) - StrIterRemainingLength(&fsi));

        FmtInfo fmt_info = {0};
        if (!parse_format_spec(spec_start, spec_len, &fmt_info)) {
            *iter = start;
            return false; // parse_format_spec already logged
        }
        if (!(fmt_info.flags & FMT_FLAG_RAW)) {
            LOG_FATAL("buf_read_fmt: only raw ({{<Nr}}/{{>Nr}}) specs allowed");
        }
        if (fmt_info.width != 1 && fmt_info.width != 2 && fmt_info.width != 4 && fmt_info.width != 8) {
            LOG_FATAL("buf_read_fmt: raw width must be 1/2/4/8 (got {})", (u64)fmt_info.width);
        }
        // Bounds check in iter space; pointer arithmetic stays inside the buffer.
        if (fmt_info.width > IterRemainingLength(iter)) {
            *iter = start;
            return false;
        }
        if (arg_index >= argc) {
            LOG_FATAL("buf_read_fmt: too few arguments for format string");
        }
        TypeSpecificIO *io = &argv[arg_index++];
        if (!io->reader) {
            LOG_FATAL("buf_read_fmt: argument {} has no reader", arg_index - 1);
        }

        // Read the raw bytes into a u64 with the spec's endianness.
        Zstr in   = (Zstr)IterPos(iter);
        u64  x    = 0;
        Zstr next = NULL;
        switch (fmt_info.width) {
            case 1 : {
                u8 v;
                next = _read_r8(in, &fmt_info, &v);
                x    = v;
                break;
            }
            case 2 : {
                u16 v;
                next = _read_r16(in, &fmt_info, &v);
                x    = v;
                break;
            }
            case 4 : {
                u32 v;
                next = _read_r32(in, &fmt_info, &v);
                x    = v;
                break;
            }
            case 8 :
                next = _read_r64(in, &fmt_info, &x);
                break;
        }
        if (!next) {
            *iter = start;
            return false;
        }
        // Audit: precondition is the `fmt_info.width > IterRemainingLength(iter)`
        // guard above; reaching here means at least `fmt_info.width` bytes remain
        // in the cursor, which is what IterMustMove consumes.
        IterMustMove(iter, fmt_info.width);

        // Resolve destination variable width via the reader fn pointer
        // (same discriminator str_read_fmt uses). Require an exact match
        // with spec width -- on-disk and in-memory layouts must agree.
        u32   var_width = 0;
        void *read_fn   = (void *)io->reader;
        if (read_fn == (void *)_read_u8 || read_fn == (void *)_read_i8) {
            var_width = 1;
        } else if (read_fn == (void *)_read_u16 || read_fn == (void *)_read_i16) {
            var_width = 2;
        } else if (read_fn == (void *)_read_u32 || read_fn == (void *)_read_i32 || read_fn == (void *)_read_f32) {
            var_width = 4;
        } else if (read_fn == (void *)_read_u64 || read_fn == (void *)_read_i64 || read_fn == (void *)_read_f64) {
            var_width = 8;
        } else {
            LOG_FATAL("buf_read_fmt: unsupported variable type at arg {}", arg_index - 1);
        }
        if (fmt_info.width != var_width) {
            LOG_FATAL(
                "buf_read_fmt: spec width {} doesn't match variable width {} at arg {}",
                (u64)fmt_info.width,
                (u64)var_width,
                arg_index - 1
            );
        }

        switch (var_width) {
            case 1 :
                *(u8 *)io->data = (u8)x;
                break;
            case 2 :
                *(u16 *)io->data = (u16)x;
                break;
            case 4 :
                *(u32 *)io->data = (u32)x;
                break;
            case 8 :
                *(u64 *)io->data = x;
                break;
        }

        StrIterMustNext(&fsi); // step over '}'
    }

    if (arg_index != argc) {
        LOG_FATAL("buf_read_fmt: {} unused argument(s) at end of format", argc - arg_index);
    }
    return true;
}

// Render one `{<Nr}` / `{>Nr}` directive into `out` (a Str backing the
// Buf body) using the same `_write_rN` helpers str_append_fmt uses for
// raw output. Returns the index of the next byte after the directive,
// or 0 on a failed write (fmt error / OOM).
static bool render_one_raw_field(Str *out, FmtInfo *fmt_info, TypeSpecificIO *io, u64 arg_index) {
    if (!io->writer) {
        LOG_FATAL("buf_*_fmt: argument {} has no writer", arg_index);
    }
    void *write_fn  = (void *)io->writer;
    u32   var_width = 0;
    if (write_fn == (void *)_write_u8 || write_fn == (void *)_write_i8) {
        var_width = 1;
    } else if (write_fn == (void *)_write_u16 || write_fn == (void *)_write_i16) {
        var_width = 2;
    } else if (write_fn == (void *)_write_u32 || write_fn == (void *)_write_i32 || write_fn == (void *)_write_f32) {
        var_width = 4;
    } else if (write_fn == (void *)_write_u64 || write_fn == (void *)_write_i64 || write_fn == (void *)_write_f64) {
        var_width = 8;
    } else {
        LOG_FATAL("buf_*_fmt: unsupported variable type at arg {}", arg_index);
    }
    if (fmt_info->width != var_width) {
        LOG_FATAL(
            "buf_*_fmt: spec width {} doesn't match variable width {} at arg {}",
            (u64)fmt_info->width,
            (u64)var_width,
            arg_index
        );
    }
    switch (fmt_info->width) {
        case 1 : {
            u8 v = *(u8 *)io->data;
            return _write_r8(out, fmt_info, &v);
        }
        case 2 : {
            u16 v = *(u16 *)io->data;
            return _write_r16(out, fmt_info, &v);
        }
        case 4 : {
            u32 v = *(u32 *)io->data;
            return _write_r32(out, fmt_info, &v);
        }
        case 8 : {
            u64 v = *(u64 *)io->data;
            return _write_r64(out, fmt_info, &v);
        }
    }
    return false;
}

// Render a binary-only fmt string + args into `out`. `out` must be a
// Str (or Str-shaped Vec(char)) backing whatever container the caller
// ultimately writes into. Shared between buf_append_fmt and the body
// rendering done by buf_write_fmt / buf_patch_fmt.
static bool render_binary_fmt(Str *out, Zstr fmtstr, TypeSpecificIO *argv, u64 argc) {
    u64     arg_index = 0;
    StrIter fsi       = StrIterFromZstr(fmtstr);
    char    fc        = 0;
    while (StrIterPeek(&fsi, &fc)) {
        if (fc != '{') {
            // Literal byte: emit verbatim (mirrors the read side's match).
            if (!StrPushBackR(out, fc)) {
                return false;
            }
            StrIterMustNext(&fsi);
            continue;
        }
        StrIterMustNext(&fsi); // step over '{'
        // `{{` escapes to a single literal '{' byte.
        char esc = 0;
        if (StrIterPeek(&fsi, &esc) && esc == '{') {
            if (!StrPushBackR(out, '{')) {
                return false;
            }
            StrIterMustNext(&fsi); // step over second '{'
            continue;
        }
        StrIter spec_start_fsi = fsi;
        Zstr    spec_start     = (Zstr)StrIterPos(&fsi);
        char    sc             = 0;
        while (StrIterPeek(&fsi, &sc) && sc != '}') {
            StrIterMustNext(&fsi);
        }
        if (sc != '}') {
            LOG_FATAL("buf_*_fmt: unterminated {{ in fmt");
        }
        u32 spec_len = (u32)(StrIterRemainingLength(&spec_start_fsi) - StrIterRemainingLength(&fsi));

        FmtInfo fmt_info = {0};
        if (!parse_format_spec(spec_start, spec_len, &fmt_info)) {
            return false;
        }
        if (!(fmt_info.flags & FMT_FLAG_RAW)) {
            LOG_FATAL("buf_*_fmt: only raw ({{<Nr}}/{{>Nr}}) specs allowed");
        }
        if (fmt_info.width != 1 && fmt_info.width != 2 && fmt_info.width != 4 && fmt_info.width != 8) {
            LOG_FATAL("buf_*_fmt: raw width must be 1/2/4/8 (got {})", (u64)fmt_info.width);
        }
        if (arg_index >= argc) {
            LOG_FATAL("buf_*_fmt: too few arguments for format string");
        }
        TypeSpecificIO *io = &argv[arg_index++];
        if (!render_one_raw_field(out, &fmt_info, io, arg_index - 1)) {
            return false;
        }
        StrIterMustNext(&fsi); // step over '}'
    }
    if (arg_index != argc) {
        LOG_FATAL("buf_*_fmt: {} unused argument(s) at end of format", argc - arg_index);
    }
    return true;
}

bool buf_append_fmt(Buf *out, Zstr fmtstr, TypeSpecificIO *argv, u64 argc) {
    if (!out || !fmtstr) {
        LOG_FATAL("buf_append_fmt: NULL out or fmtstr");
    }
    // The raw-write helpers (_write_rN) emit into a Str. Buf is Vec(u8)
    // and Str is Vec(char); both have identical layout, so we alias.
    return render_binary_fmt((Str *)out, fmtstr, argv, argc);
}

bool buf_write_fmt(Buf *out, Zstr fmtstr, TypeSpecificIO *argv, u64 argc) {
    if (!out || !fmtstr) {
        LOG_FATAL("buf_write_fmt: NULL out or fmtstr");
    }
    VecClear(out);
    return render_binary_fmt((Str *)out, fmtstr, argv, argc);
}

bool buf_patch_fmt(Buf *out, size offset, Zstr fmtstr, TypeSpecificIO *argv, u64 argc) {
    if (!out || !fmtstr) {
        LOG_FATAL("buf_patch_fmt: NULL out or fmtstr");
    }
    // Render into a scratch Str so we know the byte count before
    // mutating `out`. The patch fails if it would extend past the
    // current length -- caller must AppendFmt placeholder bytes first.
    // DefaultAllocator: rendered size is caller-controlled (open-ended).
    DefaultAllocator scratch = DefaultAllocatorInit();
    Str              tmp     = StrInit(&scratch);
    bool             ok      = render_binary_fmt(&tmp, fmtstr, argv, argc);
    if (ok) {
        if (offset > BufLength(out) || StrLen(&tmp) > BufLength(out) - offset) {
            LOG_ERROR(
                "BufPatchFmt: write of {} bytes at offset {} exceeds buf length {}",
                StrLen(&tmp),
                offset,
                BufLength(out)
            );
            ok = false;
        } else if (StrLen(&tmp)) {
            MemCopy(BufData(out) + offset, StrBegin(&tmp), StrLen(&tmp));
        }
    }
    StrDeinit(&tmp);
    DefaultAllocatorDeinit(&scratch);
    return ok;
}

void f_read_fmt(File *file, Zstr fmtstr, TypeSpecificIO *argv, u64 argc) {
    // DefaultAllocator: the slurp buffer below sizes itself from file length
    // (potentially many MiB on seekable inputs), so no stack-bound applies.
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!file || !fmtstr) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    Str buffer = StrInit(&scratch);
    i32 fd     = FileFd(file);

    // Probe seekability: if FileSeek(0, CUR) succeeds, the underlying
    // channel is positionable; if it returns -1 (ESPIPE / Windows
    // equivalent) treat the input as a stream and pull bytes until EOF.
    i64 probe = FileSeek(file, 0, FILE_SEEK_CUR);
    if (probe < 0 || fd == 0 || fd == 1 || fd == 2) {
        LOG_INFO("Reading from non-seekable channel.");
        char buf_byte = 0;
        while (FileRead(file, &buf_byte, 1) == 1) {
            StrPushBackR(&buffer, buf_byte);
        }
        str_read_fmt(StrBegin(&buffer), fmtstr, argv, argc);
    } else {
        // Remember the start position so we can rewind after a parse
        // failure, and so we can advance to "after the bytes we
        // actually consumed".
        i64 cur_pos = probe;
        i64 end_pos = FileSeek(file, 0, FILE_SEEK_END);
        if (end_pos < 0) {
            LOG_ERROR("FileSeek(END) failed during f_read_fmt");
            StrDeinit(&buffer);
            DefaultAllocatorDeinit(&scratch);
            return;
        }
        i64 file_len = end_pos - cur_pos;
        (void)FileSeek(file, cur_pos, FILE_SEEK_SET);

        if (file_len > 0) {
            StrReserve(&buffer, (u64)file_len);
            i64 got = FileRead(file, StrBegin(&buffer), (u64)file_len);
            if (got < 0) {
                LOG_ERROR("FileRead failed during f_read_fmt");
                StrDeinit(&buffer);
                DefaultAllocatorDeinit(&scratch);
                return;
            }
            StrResize(&buffer, (size)got);
        }

        if (StrLen(&buffer)) {
            Zstr new_pos = str_read_fmt(StrBegin(&buffer), fmtstr, argv, argc);
            if (!new_pos) {
                LOG_ERROR("Parse failed, rolling back...");
                (void)FileSeek(file, cur_pos, FILE_SEEK_SET);
            } else {
                // Advance the channel position past the bytes we consumed.
                i64 consumed = (i64)(new_pos - StrBegin(&buffer));
                (void)FileSeek(file, cur_pos + consumed, FILE_SEEK_SET);
            }
        }
    }

    StrDeinit(&buffer);
    DefaultAllocatorDeinit(&scratch);
}

// Forces a fixed big-endian byte iteration order so the rendered
// characters look identical on LE and BE hosts -- the format spec
// implies a wire-style representation independent of host endianness.
static inline bool write_int_as_chars(Str *o, FormatFlags flags, u64 value, size num_bytes) {
    if (!o || !num_bytes || num_bytes > 8) {
        LOG_FATAL("Invalid arguments");
    }

    bool is_caps    = (flags & FMT_FLAG_CAPS) != 0;
    bool force_case = (flags & FMT_FLAG_FORCE_CASE) != 0;

    for (size i = 0; i < num_bytes; i++) {
        u8 byte = (value >> ((num_bytes - 1 - i) * 8)) & 0xFF;

        if (IS_PRINTABLE(byte)) {
            // 'a'/'A' set force_case (caller wants the case folded);
            // 'c' leaves it clear so source bytes pass through untouched.
            if (force_case) {
                if (!StrPushBackR(o, is_caps ? TO_UPPER(byte) : TO_LOWER(byte))) {
                    return false;
                }
            } else {
                if (!StrPushBackR(o, byte)) {
                    return false;
                }
            }
        } else {
            // Render non-printables as `\xNN` so the output is
            // ASCII-safe regardless of byte content.
            u8   low = byte & 0xf;
            u8   hiw = (byte >> 4) & 0xf;
            char c1  = hiw < 10 ? '0' + hiw : is_caps ? 'A' + (hiw - 10) : 'a' + (hiw - 10);
            char c2  = low < 10 ? '0' + low : is_caps ? 'A' + (low - 10) : 'a' + (low - 10);

            if (!StrPushBackMany(o, "\\x") || !StrPushBackR(o, c1) || !StrPushBackR(o, c2)) {
                return false;
            }
        }
    }

    return true;
}

static inline bool write_char_internal(Str *o, FormatFlags flags, Zstr vs, size len) {
    if (!o || !vs || !len) {
        LOG_FATAL("Invalid arguments");
    }

    bool is_caps    = (flags & FMT_FLAG_CAPS) != 0;
    bool force_case = (flags & FMT_FLAG_FORCE_CASE) != 0;

    while (len--) {
        if (IS_PRINTABLE(*vs)) {
            // See `write_int_as_chars` for the force_case / 'c' split.
            if (force_case) {
                if (!StrPushBackR(o, is_caps ? TO_UPPER(*vs) : TO_LOWER(*vs))) {
                    return false;
                }
            } else {
                if (!StrPushBackR(o, *vs)) {
                    return false;
                }
            }
        } else {
            // Non-printable byte -> `\xHH`. Same shape as the hex-escape
            // branch above.
            u8   c   = *vs;
            u8   low = c & 0xf;
            u8   hiw = (c >> 4) & 0xf;
            char c1  = hiw < 10 ? '0' + hiw : is_caps ? 'A' + (hiw - 10) : 'a' + (hiw - 10);
            char c2  = low < 10 ? '0' + low : is_caps ? 'A' + (low - 10) : 'a' + (low - 10);
            if (!StrPushBackMany(o, "\\x") || !StrPushBackR(o, c1) || !StrPushBackR(o, c2)) {
                return false;
            }
        }
        vs++;
    }

    return true;
}

i32 ZstrHexDigitValue(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }

    return -1;
}

#if FEATURE_INT
static bool int_fmt_digit_matches_radix(char c, u8 radix) {
    i32 digit = ZstrHexDigitValue(c);

    return digit >= 0 && digit < radix;
}

static u8 int_fmt_radix_from_flags(FmtInfo *fmt_info) {
    if (fmt_info && (fmt_info->flags & FMT_FLAG_HEX)) {
        return 16;
    }
    if (fmt_info && (fmt_info->flags & FMT_FLAG_BINARY)) {
        return 2;
    }
    if (fmt_info && (fmt_info->flags & FMT_FLAG_OCTAL)) {
        return 8;
    }

    return 10;
}
#endif // FEATURE_INT

#if FEATURE_FLOAT
static bool float_fmt_uses_unsupported_flags(FmtInfo *fmt_info) {
    return fmt_info && (fmt_info->flags & (FMT_FLAG_CHAR | FMT_FLAG_HEX | FMT_FLAG_BINARY | FMT_FLAG_OCTAL |
                                           FMT_FLAG_RAW | FMT_FLAG_STRING)) != 0;
}

static bool float_fmt_append_exponent(Str *out, i64 exponent, bool uppercase) {
    char sign      = exponent < 0 ? '-' : '+';
    u64  magnitude = exponent < 0 ? (u64)(-(exponent + 1)) + 1 : (u64)exponent;

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    bool ok = true;
    StrInitStack(digits, 32) {
        char *data        = StrBegin(&digits);
        u32   digit_count = 0;

        if (magnitude == 0) {
            data[digit_count++] = '0';
        } else {
            while (magnitude > 0) {
                data[digit_count++]  = (char)('0' + (magnitude % 10));
                magnitude           /= 10;
            }
        }

        if (!StrPushBackR(out, uppercase ? 'E' : 'e') || !StrPushBackR(out, sign)) {
            ok = false;
            break;
        }

        if (digit_count < 2) {
            if (!StrPushBackR(out, '0')) {
                ok = false;
                break;
            }
        }

        while (digit_count > 0) {
            if (!StrPushBackR(out, data[--digit_count])) {
                ok = false;
                break;
            }
        }
    }

    return ok;
}

bool float_try_to_decimal_str(Str *out, Float *value, u32 precision, bool has_precision, Allocator *alloc) {
    Str canonical;
    Str result;

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    *out = StrInit(alloc);

    if (!float_try_to_str(&canonical, value, alloc)) {
        return false;
    }

    if (!has_precision) {
        *out = canonical;
        return true;
    }

    {
        Zstr body   = StrBegin(&canonical);
        Zstr dot    = NULL;
        u64  prefix = 0;
        u64  frac   = 0;

        result = StrInit(alloc);

        if (StrBegin(&canonical)[0] == '-') {
            if (!StrPushBackR(&result, '-')) {
                goto fail;
            }
            body++;
        }

        dot = ZstrFindChar(body, '.');
        if (!dot) {
            if (!StrPushBackMany(&result, body, ZstrLen(body))) {
                goto fail;
            }

            if (precision > 0) {
                if (!StrPushBackR(&result, '.')) {
                    goto fail;
                }
                for (u32 i = 0; i < precision; i++) {
                    if (!StrPushBackR(&result, '0')) {
                        goto fail;
                    }
                }
            }

            StrDeinit(&canonical);
            *out = result;
            return true;
        }

        prefix = (u64)(dot - body);
        frac   = (u64)ZstrLen(dot + 1);

        if (!StrPushBackMany(&result, body, prefix)) {
            goto fail;
        }
        if (precision > 0) {
            if (!StrPushBackR(&result, '.')) {
                goto fail;
            }
            if (!StrPushBackMany(&result, dot + 1, MIN2(frac, (u64)precision))) {
                goto fail;
            }
            for (u32 i = (u32)MIN2(frac, (u64)precision); i < precision; i++) {
                if (!StrPushBackR(&result, '0')) {
                    goto fail;
                }
            }
        }

        StrDeinit(&canonical);
        *out = result;
        return true;
    }

fail:
    StrDeinit(&canonical);
    StrDeinit(&result);
    return false;
}

bool float_try_to_scientific_str(
    Str       *out,
    Float     *value,
    u32        precision,
    bool       has_precision,
    bool       uppercase,
    Allocator *alloc
) {
    Str digits;
    Str result;
    u64 frac_digits = 0;
    i64 exponent    = 0;

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    *out = StrInit(alloc);

    if (!int_try_to_str(&digits, &value->significand, alloc)) {
        return false;
    }

    result = StrInit(alloc);

    if (FloatIsZero(value)) {
        if (value->negative) {
            if (!StrPushBackR(&result, '-')) {
                goto fail;
            }
        }
        if (!StrPushBackR(&result, '0')) {
            goto fail;
        }
        if (has_precision && precision > 0) {
            if (!StrPushBackR(&result, '.')) {
                goto fail;
            }
            for (u32 i = 0; i < precision; i++) {
                if (!StrPushBackR(&result, '0')) {
                    goto fail;
                }
            }
        }
        if (!float_fmt_append_exponent(&result, 0, uppercase)) {
            goto fail;
        }
        StrDeinit(&digits);
        *out = result;
        return true;
    }

    if (value->negative) {
        if (!StrPushBackR(&result, '-')) {
            goto fail;
        }
    }

    exponent = value->exponent + (i64)StrLen(&digits) - 1;
    if (!StrPushBackR(&result, StrBegin(&digits)[0])) {
        goto fail;
    }

    frac_digits = has_precision ? precision : (StrLen(&digits) > 0 ? StrLen(&digits) - 1 : 0);
    if (frac_digits > 0) {
        if (!StrPushBackR(&result, '.')) {
            goto fail;
        }
        for (u64 i = 0; i < frac_digits; i++) {
            if (i + 1 < StrLen(&digits)) {
                if (!StrPushBackR(&result, StrBegin(&digits)[i + 1])) {
                    goto fail;
                }
            } else {
                if (!StrPushBackR(&result, '0')) {
                    goto fail;
                }
            }
        }
    }

    if (!float_fmt_append_exponent(&result, exponent, uppercase)) {
        goto fail;
    }
    StrDeinit(&digits);
    *out = result;
    return true;

fail:
    StrDeinit(&digits);
    StrDeinit(&result);
    return false;
}

static size float_fmt_token_length(Zstr input) {
    size pos            = 0;
    bool saw_digit      = false;
    bool saw_decimal    = false;
    bool saw_exponent   = false;
    bool need_exp_digit = false;
    bool allow_sign     = true;

    if (!input) {
        LOG_FATAL("Invalid arguments");
    }

    while (input[pos]) {
        char ch = input[pos];

        if (IS_DIGIT(ch)) {
            saw_digit      = true;
            need_exp_digit = false;
            allow_sign     = false;
            pos++;
            continue;
        }

        if ((ch == '+' || ch == '-') && allow_sign) {
            allow_sign = false;
            pos++;
            continue;
        }

        if (ch == '.' && !saw_decimal && !saw_exponent) {
            saw_decimal = true;
            allow_sign  = false;
            pos++;
            continue;
        }

        if ((ch == 'e' || ch == 'E') && !saw_exponent && saw_digit) {
            saw_exponent   = true;
            need_exp_digit = true;
            allow_sign     = true;
            pos++;
            continue;
        }

        break;
    }

    if (!saw_digit || need_exp_digit) {
        return 0;
    }

    return pos;
}
#endif // FEATURE_FLOAT

// Reads up to `buffer_size` bytes from `i`, expanding `\xNN` escapes
// into the implied raw byte. Stops early on end-of-input. Returns the
// post-read cursor so the outer reader can splice it back into the
// surrounding format-walk.
static inline Zstr read_chars_internal(Zstr i, u8 *buffer, size buffer_size, FmtInfo *fmt_info) {
    if (!i || !buffer || !buffer_size) {
        LOG_FATAL("Invalid arguments");
    }

    size bytes_read = 0;
    Zstr current    = i;
    bool force_case = fmt_info && (fmt_info->flags & FMT_FLAG_FORCE_CASE) != 0;
    bool is_caps    = fmt_info && (fmt_info->flags & FMT_FLAG_CAPS) != 0;

    while (bytes_read < buffer_size && *current && !IS_SPACE(*current)) {
        u8 char_to_store;

        if (current[0] == '\\' && current[1] == 'x') {
            // Probe the two hex bytes WITHOUT walking past the NUL.
            // Input is Zstr (NUL-terminated); a payload of "\\x" or
            // "\\xN" would otherwise let `hex_byte(current[2], current[3])`
            // read one or two bytes past the terminator.
            i32 hex_val = -1;
            if (current[2] != '\0' && current[3] != '\0') {
                hex_val = hex_byte(current[2], current[3]);
            }
            if (hex_val >= 0) {
                char_to_store  = (u8)hex_val;
                current       += 4;
            } else {
                // Bad nibbles (or truncated `\x`): salvage the `\` as a
                // literal byte so the rest of the field still consumes;
                // the broken tail surfaces in the caller's buffer instead
                // of being silently swallowed.
                char_to_store = (u8)*current;
                current++;
            }
        } else {
            char_to_store = (u8)*current;
            current++;
        }

        if (force_case) {
            char_to_store = is_caps ? TO_UPPER(char_to_store) : TO_LOWER(char_to_store);
        }

        buffer[bytes_read] = char_to_store;
        bytes_read++;
    }

    return current;
}

// Buf is Vec(u8), Str is Vec(char) -- identical layout, so Buf text I/O
// aliases the Str path (same (Str *) cast buf_append_fmt relies on).
bool _write_Buf(Str *o, FmtInfo *fmt_info, Buf *b) {
    return _write_Str(o, fmt_info, (Str *)b);
}
Zstr _read_Buf(Zstr i, FmtInfo *fmt_info, Buf *b) {
    return _read_Str(i, fmt_info, (Str *)b);
}

// DateTime renders / parses as ISO 8601: `YYYY-MM-DDTHH:MM:SS[.fffffffff]`
// followed by `Z` (UTC) or `+HH:MM` / `-HH:MM`. The fractional part is
// emitted only when nanoseconds are nonzero; the 9-digit zero-padded
// fraction is exactly the nanosecond count, so it parses back as a plain
// integer. Writer and reader share the layout, so a value round-trips.
bool _write_DateTime(Str *o, FmtInfo *fmt_info, DateTime *dt) {
    (void)fmt_info;
    if (!o || !dt) {
        LOG_FATAL("Invalid arguments");
    }
    if (!StrAppendFmt(
            o,
            "{04}-{02}-{02}T{02}:{02}:{02}",
            dt->year,
            (u32)dt->month,
            (u32)dt->day,
            (u32)dt->hour,
            (u32)dt->minute,
            (u32)dt->second
        )) {
        return false;
    }
    if (dt->nanosecond != 0 && !StrAppendFmt(o, ".{09}", dt->nanosecond)) {
        return false;
    }
    if (dt->utc_offset_seconds == 0) {
        return StrAppendFmt(o, "Z");
    }
    i32 off = dt->utc_offset_seconds;
    u32 ao  = (u32)(off < 0 ? -off : off);
    u32 oh  = ao / 3600;
    u32 om  = (ao % 3600) / 60;
    if (off < 0) {
        return StrAppendFmt(o, "-{02}:{02}", oh, om);
    }
    return StrAppendFmt(o, "+{02}:{02}", oh, om);
}

Zstr _read_DateTime(Zstr i, FmtInfo *fmt_info, DateTime *dt) {
    (void)fmt_info;
    if (!i || !dt) {
        LOG_FATAL("Invalid arguments");
    }
    Zstr p    = i;
    i32  year = 0;
    u32  mon = 0, day = 0, hh = 0, mm = 0, ss = 0;
    StrReadFmt(p, "{}-{}-{}T{}:{}:{}", year, mon, day, hh, mm, ss);
    if (p == i) {
        return i; // no date-time prefix matched
    }

    u32 nanos = 0;
    if (*p == '.') {
        p++;
        Zstr frac_at = p;
        StrReadFmt(p, "{}", nanos);
        if (p == frac_at) {
            return i; // '.' with no fraction
        }
    }

    i32 offset = 0;
    if (*p == 'Z') {
        p++;
    } else if (*p == '+' || *p == '-') {
        i32  sign   = (*p == '-') ? -1 : 1;
        Zstr off_at = ++p;
        u32  oh = 0, om = 0;
        StrReadFmt(p, "{}:{}", oh, om);
        if (p == off_at) {
            return i; // malformed offset
        }
        offset = sign * (i32)(oh * 3600 + om * 60);
    } else {
        return i; // missing offset designator
    }

    DateTime tmp;
    tmp.year               = year;
    tmp.month              = (u8)mon;
    tmp.day                = (u8)day;
    tmp.hour               = (u8)hh;
    tmp.minute             = (u8)mm;
    tmp.second             = (u8)ss;
    tmp.nanosecond         = nanos;
    tmp.utc_offset_seconds = offset;
    tmp.weekday            = 0;
    // Derive the weekday from the canonical conversion so a parsed value
    // carries a correct weekday regardless of the input text.
    tmp.weekday = DateTimeFromUnixNs(DateTimeToUnixNs(tmp), offset).weekday;
    *dt         = tmp;
    return p;
}

bool _write_Str(Str *o, FmtInfo *fmt_info, Str *s) {
    if (!o || !s || !fmt_info) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateStr(o);
    ValidateStr(s);

    // Remember the pre-render length so the post-write padding step
    // can compute "how much content did we just emit" without
    // accounting for prior contents.
    size start_len = StrLen(o);

    if (StrLen(s)) {
        if (fmt_info->flags & FMT_FLAG_HEX) {
            // `{x}` over a Str renders each byte as `0xNN` with a
            // single-space separator (no separator before the first
            // byte). Two hex digits per byte, zero-padded.
            StrIntFormat config = {.base = 16, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0};
            StrForeachIdx(s, c, i) {
                if (i > 0) {
                    if (!StrPushBackR(o, ' ')) {
                        return false;
                    }
                }
                Str hex = StrInit(StrAllocator(o));
                if (!StrFromU64(&hex, c, &config)) {
                    StrDeinit(&hex);
                    return false;
                }
                if (StrLen(&hex) == 1) {
                    if (!StrPushFrontR(&hex, '0')) {
                        StrDeinit(&hex);
                        return false;
                    }
                }
                if (!StrPushBackMany(o, "0x") || !StrMerge(o, &hex)) {
                    StrDeinit(&hex);
                    return false;
                }
                StrDeinit(&hex);
            }
        } else {
            // `precision` truncates to at most N chars (mirrors Python's
            // `{:.Ns}`); precision 0 is the explicit "render nothing"
            // case, not an error.
            size len = StrLen(s);
            if (fmt_info->flags & FMT_FLAG_HAS_PRECISION) {
                if (fmt_info->precision == 0) {
                    len = 0;
                } else {
                    len = MIN2(len, fmt_info->precision);
                }
            }

            if (fmt_info->flags & FMT_FLAG_CHAR) {
                if (!write_char_internal(o, fmt_info->flags, (Zstr)StrBegin(s), len)) {
                    return false;
                }
            } else {
                StrForeachInRange(s, c, 0, len) {
                    if (IS_PRINTABLE(c)) {
                        if (!StrPushBackR(o, c)) {
                            return false;
                        }
                    } else {
                        Zstr digits = "0123456789abcdef";
                        if (!StrPushBackMany(o, "\\x") || !StrPushBackR(o, digits[(c >> 4) & 0xf]) ||
                            !StrPushBackR(o, digits[c & 0xf])) {
                            return false;
                        }
                    }
                }
            }
        }
    }

    if (fmt_info->width > 0) {
        size content_len = StrLen(o) - start_len;
        if (!StrPad(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}

bool _write_Zstr(Str *o, FmtInfo *fmt_info, Zstr *s) {
    if (!o || !s || !*s || !fmt_info) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    ValidateStr(o);

    // Snapshot the pre-render length so post-render padding sees the
    // size of just-this-field, not the accumulated buffer.
    size start_len = StrLen(o);
    Zstr xs        = *s;

    // Empty Zstr: skip the body and fall straight through to padding.
    if (xs[0] != '\0') {
        if (fmt_info->flags & FMT_FLAG_HEX) {
            // Same `0xNN`-per-byte rendering as the Str path above;
            // walked through the Zstr terminator instead of `.length`.
            size i = 0;
            while (xs[i]) {
                if (i > 0) {
                    if (!StrPushBackR(o, ' ')) {
                        return false;
                    }
                }
                Str          hex    = StrInit(StrAllocator(o));
                StrIntFormat config = {.base = 16, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0};
                if (!StrFromU64(&hex, (u8)xs[i], &config)) {
                    StrDeinit(&hex);
                    return false;
                }
                if (StrLen(&hex) == 1) {
                    if (!StrPushFrontR(&hex, '0')) {
                        StrDeinit(&hex);
                        return false;
                    }
                }
                if (!StrPushBackMany(o, "0x") || !StrMerge(o, &hex)) {
                    StrDeinit(&hex);
                    return false;
                }
                StrDeinit(&hex);
                i++;
            }
        } else {
            // Precision truncates -- see the Str path above for the
            // precision-0 carve-out.
            size len = ZstrLen(xs);
            if (fmt_info->flags & FMT_FLAG_HAS_PRECISION) {
                if (fmt_info->precision == 0) {
                    len = 0;
                } else {
                    len = MIN2(len, fmt_info->precision);
                }
            }

            if (fmt_info->flags & FMT_FLAG_CHAR) {
                if (!write_char_internal(o, fmt_info->flags, xs, len)) {
                    return false;
                }
            } else {
                for (size i = 0; i < len; i++) {
                    if (IS_PRINTABLE(xs[i])) {
                        if (!StrPushBackR(o, xs[i])) {
                            return false;
                        }
                    } else {
                        Zstr digits = "0123456789abcdef";
                        if (!StrPushBackMany(o, "\\x") || !StrPushBackR(o, digits[(xs[i] >> 4) & 0xf]) ||
                            !StrPushBackR(o, digits[xs[i] & 0xf])) {
                            return false;
                        }
                    }
                }
            }
        }
    }

    if (fmt_info->width > 0) {
        size content_len = StrLen(o) - start_len;
        if (!StrPad(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}

bool _write_ZstrAlloc(Str *o, FmtInfo *fmt_info, ZstrIOArg *arg) {
    Zstr *value = NULL;

    if (!arg) {
        LOG_FATAL("Invalid arguments");
    }

    value = (Zstr *)arg->value;
    return _write_Zstr(o, fmt_info, value);
}

bool _write_u64(Str *o, FmtInfo *fmt_info, u64 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 8);
    }

    // Snapshot the pre-render length so post-render padding sees the
    // size of just-this-field, not the accumulated buffer.
    size start_len = StrLen(o);

    // Format into a scratch Str first, then merge into `o`: keeps the
    // base-prefix / sign / padding logic from having to reach into
    // `o`'s prefix bytes after the fact.
    Str temp = StrInit(StrAllocator(o));

    u8 base = 10;
    if (fmt_info->flags & FMT_FLAG_HEX) {
        base = 16;
    } else if (fmt_info->flags & FMT_FLAG_BINARY) {
        base = 2;
    } else if (fmt_info->flags & FMT_FLAG_OCTAL) {
        base = 8;
    }

    // Zero-pad mode suppresses any base prefix so the rendered glyph
    // count matches the requested width exactly (e.g. {016x} -> 16 hex
    // chars, not "0x" + 14 chars).
    bool         zero_pad   = (fmt_info->flags & FMT_FLAG_ZERO_PAD) != 0;
    bool         use_prefix = (base != 10) && !zero_pad;
    StrIntFormat config = {.base = base, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0, .use_prefix = use_prefix};
    if (!StrFromU64(&temp, *v, &config)) {
        StrDeinit(&temp);
        return false;
    }

    if (!StrMerge(o, &temp)) {
        StrDeinit(&temp);
        return false;
    }
    StrDeinit(&temp);

    if (fmt_info->width > 0) {
        size content_len = StrLen(o) - start_len;
        if (zero_pad) {
            if (!pad_numeric_zeros(o, start_len, fmt_info->width, content_len)) {
                return false;
            }
        } else if (!StrPad(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}

bool _write_u32(Str *o, FmtInfo *fmt_info, u32 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 4);
    }

    u64 val = *v;
    return _write_u64(o, fmt_info, &val);
}

bool _write_u16(Str *o, FmtInfo *fmt_info, u16 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 2);
    }

    u64 val = *v;
    return _write_u64(o, fmt_info, &val);
}

bool _write_u8(Str *o, FmtInfo *fmt_info, u8 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 1);
    }

    u64 vx = *v;
    return _write_u64(o, fmt_info, &vx);
}

bool _write_i64(Str *o, FmtInfo *fmt_info, i64 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 8);
    }

    // Snapshot the pre-render length so post-render padding sees the
    // size of just-this-field, not the accumulated buffer.
    size start_len = StrLen(o);

    // Format into a scratch Str first, then merge into `o`: keeps the
    // base-prefix / sign / padding logic from having to reach into
    // `o`'s prefix bytes after the fact.
    Str temp = StrInit(StrAllocator(o));

    u8 base = 10;
    if (fmt_info->flags & FMT_FLAG_HEX) {
        base = 16;
    } else if (fmt_info->flags & FMT_FLAG_BINARY) {
        base = 2;
    } else if (fmt_info->flags & FMT_FLAG_OCTAL) {
        base = 8;
    }

    // Zero-pad suppresses the base prefix; the leading sign (if any)
    // is kept ahead of the '0' fill -- see pad_numeric_zeros.
    bool         zero_pad   = (fmt_info->flags & FMT_FLAG_ZERO_PAD) != 0;
    bool         use_prefix = (base != 10) && !zero_pad;
    StrIntFormat config = {.base = base, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0, .use_prefix = use_prefix};
    if (!StrFromI64(&temp, *v, &config)) {
        StrDeinit(&temp);
        return false;
    }

    if (!StrMerge(o, &temp)) {
        StrDeinit(&temp);
        return false;
    }
    StrDeinit(&temp);

    if (fmt_info->width > 0) {
        size content_len = StrLen(o) - start_len;
        if (zero_pad) {
            if (!pad_numeric_zeros(o, start_len, fmt_info->width, content_len)) {
                return false;
            }
        } else if (!StrPad(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}

bool _write_i32(Str *o, FmtInfo *fmt_info, i32 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 4);
    }

    i64 val = *v;
    return _write_i64(o, fmt_info, &val);
}

bool _write_i16(Str *o, FmtInfo *fmt_info, i16 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 2);
    }

    i64 vx = *v;
    return _write_i64(o, fmt_info, &vx);
}

bool _write_i8(Str *o, FmtInfo *fmt_info, i8 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 1);
    }

    i64 vx = *v;
    return _write_i64(o, fmt_info, &vx);
}

bool _write_f64(Str *o, FmtInfo *fmt_info, f64 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    // {c} on a float reinterprets the IEEE 754 byte pattern as 8
    // chars (high byte first). Goes through MemCopy rather than a
    // union cast so the aliasing rules stay clean across compilers.
    if (fmt_info->flags & FMT_FLAG_CHAR) {
        u64 bits;
        MemCopy(&bits, v, sizeof(bits));
        return write_int_as_chars(o, fmt_info->flags, bits, 8);
    }

    // Snapshot the pre-render length so post-render padding sees the
    // size of just-this-field, not the accumulated buffer.
    size start_len = StrLen(o);

    // NaN / +/-inf bypass `StrFromF64` so the rendered token stays
    // stable ("nan" / "inf" / "NAN" / "INF") regardless of how the
    // generic float renderer would print them.
    if (F64IsNan(*v)) {
        if (fmt_info->flags & FMT_FLAG_CAPS) {
            if (!StrPushBackMany(o, "F64_NAN")) {
                return false;
            }
        } else {
            if (!StrPushBackMany(o, "nan")) {
                return false;
            }
        }
    } else if (F64IsInf(*v)) {
        if (*v < 0) {
            if (!StrPushBackR(o, '-')) {
                return false;
            }
        }

        if (fmt_info->flags & FMT_FLAG_CAPS) {
            if (!StrPushBackMany(o, "INF")) {
                return false;
            }
        } else {
            if (!StrPushBackMany(o, "inf")) {
                return false;
            }
        }
    } else {
        // Format into a scratch Str first then merge -- same rationale
        // as the integer renderers: padding/sign accounting wants a
        // self-contained slice it can measure.
        Str temp = StrInit(StrAllocator(o));

        u8             precision = fmt_info->flags & FMT_FLAG_HAS_PRECISION ? fmt_info->precision : 6;
        StrFloatFormat config    = {
               .precision = precision,
               .force_sci = (fmt_info->flags & FMT_FLAG_SCIENTIFIC) != 0,
               .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0
        };
        if (!StrFromF64(&temp, *v, &config)) {
            StrDeinit(&temp);
            return false;
        }

        if (!StrMerge(o, &temp)) {
            StrDeinit(&temp);
            return false;
        }
        StrDeinit(&temp);
    }

    if (fmt_info->width > 0) {
        size content_len = StrLen(o) - start_len;
        if (!StrPad(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}

bool _write_f32(Str *o, FmtInfo *fmt_info, f32 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    // {c} on f32: same byte-pattern reinterpret as the f64 path, sized
    // down to 4 chars.
    if (fmt_info->flags & FMT_FLAG_CHAR) {
        u32 bits;
        MemCopy(&bits, v, sizeof(bits));
        return write_int_as_chars(o, fmt_info->flags, bits, 4);
    }

    f64 val = *v;
    return _write_f64(o, fmt_info, &val);
}

#if FEATURE_FLOAT
bool _write_Float(Str *o, FmtInfo *fmt_info, Float *value) {
    size start_len = 0;
    Str  temp;

    if (!o || !fmt_info || !value) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    ValidateStr(o);
    ValidateFloat(value);

    if (float_fmt_uses_unsupported_flags(fmt_info)) {
        LOG_FATAL("Float only supports decimal and scientific formatting");
    }

    start_len = StrLen(o);
    if (fmt_info->flags & FMT_FLAG_SCIENTIFIC) {
        if (!float_try_to_scientific_str(
                &temp,
                value,
                fmt_info->precision,
                (fmt_info->flags & FMT_FLAG_HAS_PRECISION) != 0,
                (fmt_info->flags & FMT_FLAG_CAPS) != 0,
                StrAllocator(o)
            )) {
            return false;
        }
    } else {
        if (!float_try_to_decimal_str(
                &temp,
                value,
                fmt_info->precision,
                (fmt_info->flags & FMT_FLAG_HAS_PRECISION) != 0,
                StrAllocator(o)
            )) {
            return false;
        }
    }

    if (!StrMerge(o, &temp)) {
        StrDeinit(&temp);
        return false;
    }
    StrDeinit(&temp);

    if (fmt_info->width > 0) {
        size content_len = StrLen(o) - start_len;
        if (!StrPad(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}
#endif // FEATURE_FLOAT

char ZstrProcessEscape(Zstr *str) {
    if (!str || !*str)
        return 0;

    Zstr s = *str;
    if (*s != '\\') {
        LOG_ERROR("ZstrProcessEscape called on non-escape sequence");
        return 0;
    }

    s++; // Skip backslash
    char result = 0;

    switch (*s) {
        case 'n' :
            result = '\n';
            break;
        case 'r' :
            result = '\r';
            break;
        case 't' :
            result = '\t';
            break;
        case 'b' :
            result = '\b';
            break;
        case 'f' :
            result = '\f';
            break;
        case 'v' :
            result = '\v';
            break;
        case 'a' :
            result = '\a';
            break;
        case '\\' :
            result = '\\';
            break;
        case '"' :
            result = '"';
            break;
        case '\'' :
            result = '\'';
            break;
        case '0' :
            result = '\0';
            break;
        case 'x' : { // Hex escape
            s++;
            // Refuse a truncated `\x` / `\xN` at the end of input rather
            // than letting `hex_byte` peek past the NUL terminator.
            if (s[0] == '\0' || s[1] == '\0') {
                LOG_ERROR("Invalid hex escape sequence");
                return 0;
            }
            i32 hex_val = hex_byte(s[0], s[1]);
            if (hex_val < 0) {
                LOG_ERROR("Invalid hex escape sequence");
                return 0;
            }
            result = (char)hex_val;
            s++; // Point to second hex digit
            break;
        }
        default :
            LOG_ERROR("Invalid escape sequence '\\{c}'", s[0]);
            return 0;
    }

    *str = s; // Update pointer to last processed character
    return result;
}

Zstr _read_Str(Zstr i, FmtInfo *fmt_info, Str *s) {
    if (!i || !s)
        LOG_FATAL("Invalid arguments");

    ValidateStr(s);

    bool force_case = fmt_info && (fmt_info->flags & FMT_FLAG_FORCE_CASE) != 0;
    bool is_caps    = fmt_info && (fmt_info->flags & FMT_FLAG_CAPS) != 0;
    bool is_string  = fmt_info && (fmt_info->flags & FMT_FLAG_STRING) != 0;
    u32  r          = fmt_info->max_read_len;

    if (!*i || !r) {
        LOG_ERROR("Empty input string");
        return i;
    }

    char quote = 0;
    if (is_string && (*i == '"' || *i == '\'')) {
        quote = *i++;
        r--;
    }

    while (r && *i) {
        if (quote) {
            if (*i == '\\') {
                Zstr curr = i;
                char c    = ZstrProcessEscape(&curr);
                if (c == 0) {
                    StrDeinit(s);
                    return NULL;
                }
                i = curr + 1;
                // `r` is u32. Subtracting 2 when r == 1 wraps to
                // 0xFFFFFFFE, turning the bounded read into ~4 GB.
                // The escape consumed at least 2 input chars but the
                // caller may have set max_read_len = 1.
                r = (r >= 2u) ? (r - 2u) : 0u;

                if (force_case) {
                    c = is_caps ? TO_UPPER(c) : TO_LOWER(c);
                }

                StrPushBackR(s, c);
            } else if (*i == quote) {
                i++;
                r--;
                return i;
            } else {
                char c = *i++;
                r--;

                if (force_case) {
                    c = is_caps ? TO_UPPER(c) : TO_LOWER(c);
                }

                StrPushBackR(s, c);
            }
        } else {
            // Unquoted form: whitespace ends the field.
            if (is_string && IS_SPACE(*i)) {
                return i;
            }

            if (*i == '\\') {
                Zstr curr = i;
                char c    = ZstrProcessEscape(&curr);
                if (c == 0) {
                    StrDeinit(s);
                    return NULL;
                }
                i = curr + 1;
                // See the quoted branch above for the r-saturation reason.
                r = (r >= 2u) ? (r - 2u) : 0u;

                if (force_case) {
                    c = is_caps ? TO_UPPER(c) : TO_LOWER(c);
                }

                StrPushBackR(s, c);
            } else {
                char c = *i++;
                r--;

                if (force_case) {
                    c = is_caps ? TO_UPPER(c) : TO_LOWER(c);
                }

                StrPushBackR(s, c);
            }
        }
    }

    if (quote) {
        LOG_ERROR("Unterminated quoted string");
        StrDeinit(s);
        return NULL;
    }

    return i;
}

// Permissive accept-set for the digit-collection pre-pass. Hex
// letters, base-prefix letters, and the float exponent introducer all
// pass; the strict per-base validation runs downstream after we know
// the token's shape.
static bool is_valid_number_char(char c, bool is_first_char, bool allow_decimal) {
    if (IS_DIGIT(c))
        return true;

    if ((c == '+' || c == '-') && is_first_char)
        return true;

    if (c == '.' && allow_decimal)
        return true;

    // Second character of a "0x"/"0b"/"0o" prefix. The preceding '0' /
    // base detection happens in the parser proper, so accepting the
    // letter here just delegates the real validation downstream.
    if (!is_first_char && (c == 'x' || c == 'X' || c == 'b' || c == 'B' || c == 'o' || c == 'O')) {
        return true;
    }

    // Hex letters are accepted unconditionally here; the parser proper
    // gates them by the active base.
    if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
        return true;
    }

    if (allow_decimal && (c == 'e' || c == 'E'))
        return true;

    return false;
}

// Strict re-validation after `is_valid_number_char` collected the
// candidate slice. Rejects bare base prefixes, doubled signs, and
// (when `allow_float` is false) any decimal/exponent residue, before
// the slice is handed to the typed `StrToU64` / `StrToI64` parser.
static bool is_valid_numeric_string(const Str *str, bool allow_float) {
    if (!str || !StrBegin(str))
        return false;

    size        len  = StrLen(str);
    const char *data = StrBegin(str);

    if (len == 0)
        return false;

    // `inf`, `nan`, `-inf` short-circuit the digit-walk: those are the
    // only legal float tokens with no digit content.
    if (allow_float) {
        if (len == 3) {
            if ((data[0] == 'i' || data[0] == 'I') && (data[1] == 'n' || data[1] == 'N') &&
                (data[2] == 'f' || data[2] == 'F')) {
                return true;
            }
            if ((data[0] == 'n' || data[0] == 'N') && (data[1] == 'a' || data[1] == 'A') &&
                (data[2] == 'n' || data[2] == 'N')) {
                return true;
            }
        }

        if (len == 4 && data[0] == '-') {
            if ((data[1] == 'i' || data[1] == 'I') && (data[2] == 'n' || data[2] == 'N') &&
                (data[3] == 'f' || data[3] == 'F')) {
                return true;
            }
        }
    }

    bool is_hex = false;
    bool is_bin = false;
    bool is_oct = false;

    if (len > 2 && data[0] == '0') {
        if (data[1] == 'x' || data[1] == 'X') {
            is_hex = true;
        } else if (data[1] == 'b' || data[1] == 'B') {
            is_bin = true;
        } else if (data[1] == 'o' || data[1] == 'O') {
            is_oct = true;
        }
    }

    bool has_decimal = false;
    bool has_exp     = false;

    for (size i = 0; i < len; i++) {
        char c = data[i];

        // The two-char base prefix is consumed up front; the digit
        // pass below validates everything after it.
        if ((is_hex || is_bin || is_oct) && (i == 0 || i == 1)) {
            continue;
        }

        // Signs are legal at position 0 and immediately after the float
        // exponent introducer; nowhere else.
        if ((c == '+' || c == '-') &&
            (i == 0 || (allow_float && has_exp && (i > 0 && (data[i - 1] == 'e' || data[i - 1] == 'E'))))) {
            continue;
        }

        // At most one decimal point and at most one exponent are legal.
        if (allow_float && c == '.') {
            if (has_decimal) {
                return false;
            }
            has_decimal = true;
            continue;
        }

        if (allow_float && (c == 'e' || c == 'E')) {
            if (has_exp) {
                return false;
            }
            has_exp = true;
            continue;
        }

        if (c >= '0' && c <= '9') {
            continue;
        }

        if (is_hex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            continue;
        }

        if (is_bin && (c == '0' || c == '1')) {
            continue;
        }

        if (is_oct && (c >= '0' && c <= '7')) {
            continue;
        }

        return false;
    }

    // Trailing `e`/`E`/`+`/`-` is an incomplete exponent.
    if (allow_float) {
        if (has_exp) {
            char last_char = data[len - 1];
            if (last_char == 'e' || last_char == 'E' || last_char == '+' || last_char == '-') {
                return false;
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// `_read_*` type-specific readers
//
// Each reader scans a numeric / string slice out of the input, copies it
// into a temporary Str, runs the validate-and-convert pipeline, and
// returns the consumed end. The per-call DefaultAllocator is the right
// fit here:
//
//   - The TypeSpecificReader function-pointer signature has no Allocator *
//     slot (it's part of the public visitor protocol), so we cannot
//     accept one from the caller without changing public API.
//   - Slice lengths are caller-controlled (the input string may be
//     arbitrarily large), so a stack-bound StrInitStack would impose a
//     truncation cap that the existing pipeline never had.
//   - DefaultAllocator stays as DebugAllocator in debug builds, so leak
//     / canary instrumentation still covers these hot paths.
// ---------------------------------------------------------------------------

Zstr _read_f64(Zstr i, FmtInfo *fmt_info, f64 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        u64  temp = 0;
        Zstr next = read_chars_internal(i, (u8 *)&temp, sizeof(temp), fmt_info);
        *v        = (f64)temp;
        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    StrIter si = StrIterFromZstr(i);
    char    c  = 0;

    while (StrIterPeek(&si, &c) && IS_SPACE(c)) {
        StrIterMustNext(&si);
    }

    if (!StrIterRemainingLength(&si)) {
        LOG_ERROR("Failed to parse f64: empty input");
        DefaultAllocatorDeinit(&scratch);
        return StrIterDataAt(&si, StrIterIndex(&si));
    }

    // `inf` / `nan` / `-inf` / `-nan` need a wider token-scan than
    // the digit-by-digit number path; peek one ahead so a leading `-`
    // followed by `i`/`I` enters this branch too.
    char c1 = 0;
    (void)StrIterPeekAt(&si, 1, &c1);
    if ((c == 'i' || c == 'I' || c == 'n' || c == 'N') || (c == '-' && (c1 == 'i' || c1 == 'I'))) {
        StrIter saved = si;
        Zstr    start = StrIterDataAt(&si, StrIterIndex(&si));
        while (StrIterPeek(&si, &c) && !IS_SPACE(c)) {
            StrIterMustNext(&si);
        }

        Str temp = StrInitFromCstr(start, (size)(StrIterIndex(&si) - StrIterIndex(&saved)), &scratch);

        // Special-value tokens (`inf`, `nan`, ...) carry through the
        // generic Str-to-float path; non-matching tokens fall back to
        // the digit-by-digit scan below.
        if (StrToF64(&temp, v, NULL)) {
            StrDeinit(&temp);
            DefaultAllocatorDeinit(&scratch);
            return StrIterDataAt(&si, StrIterIndex(&si));
        }
        StrDeinit(&temp);

        si = saved;
    }

    StrIter saved       = si;
    bool    has_decimal = false;

    while (StrIterPeek(&si, &c)) {
        // A second '.' ends the number; the parser proper rejects
        // multi-dot mantissas, so stopping here keeps the rejected tail
        // unconsumed for the caller.
        if (c == '.') {
            if (has_decimal)
                break;
            has_decimal = true;
        }

        // Exponent introducer: consume the optional sign and require at
        // least one digit, otherwise rewind so the 'e' is left to the
        // caller's tail (e.g. a unit suffix).
        if ((c == 'e' || c == 'E') && StrIterIndex(&si) > StrIterIndex(&saved)) {
            StrIterMustNext(&si);

            if (StrIterPeek(&si, &c) && (c == '+' || c == '-')) {
                StrIterMustNext(&si);
            }

            if (!StrIterPeek(&si, &c) || !IS_DIGIT(c)) {
                StrIterMustPrev(&si);
                break;
            }

            while (StrIterPeek(&si, &c) && IS_DIGIT(c)) {
                StrIterMustNext(&si);
            }
            break;
        }

        if (!is_valid_number_char(c, StrIterIndex(&si) == StrIterIndex(&saved), true)) {
            break;
        }

        StrIterMustNext(&si);
    }

    Zstr start = StrIterDataAt(&si, StrIterIndex(&saved));
    size pos   = StrIterIndex(&si) - StrIterIndex(&saved);

    Str temp = StrInitFromCstr(start, pos, &scratch);

    if (!is_valid_numeric_string(&temp, true)) {
        LOG_ERROR("Invalid floating point format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    if (!StrToF64(&temp, v, NULL)) {
        LOG_ERROR("Failed to parse f64");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

Zstr _read_u8(Zstr i, FmtInfo *fmt_info, u8 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    StrIter si = StrIterFromZstr(i);
    char    c  = 0;

    while (StrIterPeek(&si, &c) && IS_SPACE(c)) {
        StrIterMustNext(&si);
    }

    if (!StrIterRemainingLength(&si)) {
        LOG_ERROR("Failed to parse u8: empty input");
        DefaultAllocatorDeinit(&scratch);
        return StrIterDataAt(&si, StrIterIndex(&si));
    }

    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        Zstr next = read_chars_internal(StrIterDataAt(&si, StrIterIndex(&si)), (u8 *)v, sizeof(*v), fmt_info);
        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    StrIter saved = si;

    while (StrIterPeek(&si, &c)) {
        if (!is_valid_number_char(c, StrIterIndex(&si) == StrIterIndex(&saved), false)) {
            break;
        }

        StrIterMustNext(&si);
    }

    Zstr start = StrIterDataAt(&si, StrIterIndex(&saved));
    size pos   = StrIterIndex(&si) - StrIterIndex(&saved);

    Str temp = StrInitFromCstr(start, pos, &scratch);

    // A bare base prefix ("0x", "0b", "0o") with no following digits is
    // a malformed integer; the parser proper would accept the leading
    // '0' and silently lose the prefix, so reject early.
    if (StrLen(&temp) == 2 && StrBegin(&temp)[0] == '0' &&
        (StrBegin(&temp)[1] == 'x' || StrBegin(&temp)[1] == 'X' || StrBegin(&temp)[1] == 'b' ||
         StrBegin(&temp)[1] == 'B' || StrBegin(&temp)[1] == 'o' || StrBegin(&temp)[1] == 'O')) {
        LOG_ERROR("Incomplete number format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    if (!is_valid_numeric_string(&temp, false)) {
        LOG_ERROR("Invalid numeric format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // `StrToU64` auto-detects the base from a `0x` / `0b` / `0o`
    // prefix; the `is_valid_numeric_string` check above already
    // rejected a bare prefix with no digits.
    u64 val;
    if (!StrToU64(&temp, &val, NULL)) {
        LOG_ERROR("Failed to parse u8");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    if (val > UINT8_MAX) {
        LOG_ERROR("Value {} exceeds u8 maximum ({})", val, UINT8_MAX);
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    *v = (u8)val;
    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

// ---------------------------------------------------------------------------
// Text-mode integer readers: _read_u16 / _read_u32 / _read_u64 / _read_i8 /
// _read_i16 / _read_i32 / _read_i64.
//
// Bodies were near-identical -- 7 functions, ~90 lines each, ~700 lines of
// near-duplicate code -- and the function pointers are compared by identity
// at lines ~695-712 / ~903-913 (`read_fn == (void *)_read_u8`, etc.), so
// they MUST remain distinct symbols. Stamp them out with a macro that
// parameterises the type / parser / bound-check; each instance compiles to
// its own function with its own address.
//
// `_read_u8` is the outlier (skips whitespace BEFORE checking the char-format
// flag, where all 7 others check the flag against the original cursor first)
// and is kept as a stand-alone definition below for clarity. The behaviour
// asymmetry is exercised by the format tests, so unifying it would change
// observable behaviour.
//
// Macro parameters:
//   NAME         function suffix (u16, i32, ...)
//   T            destination type (u16, i32, ...)
//   VAL_T        parser's natural result type (u64 for unsigned, i64 for signed)
//   PARSER       StrToU64 / StrToI64
//   BOUND_CHECK  block that runs after the parser returns, emitting
//                LOG_ERROR + cleanup + early-return on out-of-range; empty
//                for u64 / i64 where the parser's natural range == T's range
// ---------------------------------------------------------------------------
#define _MAKE_READ_TXT_INT(NAME, T, VAL_T, PARSER, BOUND_CHECK)                                                        \
    Zstr _read_##NAME(Zstr i, FmtInfo *fmt_info, T *v) {                                                               \
        DefaultAllocator scratch = DefaultAllocatorInit();                                                             \
                                                                                                                       \
        if (!i || !v) {                                                                                                \
            DefaultAllocatorDeinit(&scratch);                                                                          \
            LOG_FATAL("Invalid arguments");                                                                            \
        }                                                                                                              \
                                                                                                                       \
        if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {                                                           \
            *v        = 0;                                                                                             \
            Zstr next = read_chars_internal(i, (u8 *)v, sizeof(*v), fmt_info);                                         \
            DefaultAllocatorDeinit(&scratch);                                                                          \
            return next;                                                                                               \
        }                                                                                                              \
                                                                                                                       \
        StrIter si = StrIterFromZstr(i);                                                                               \
        char    c  = 0;                                                                                                \
                                                                                                                       \
        while (StrIterPeek(&si, &c) && IS_SPACE(c)) {                                                                  \
            StrIterMustNext(&si);                                                                                      \
        }                                                                                                              \
                                                                                                                       \
        if (!StrIterRemainingLength(&si)) {                                                                            \
            LOG_ERROR("Failed to parse " #NAME ": empty input");                                                       \
            DefaultAllocatorDeinit(&scratch);                                                                          \
            return StrIterDataAt(&si, StrIterIndex(&si));                                                              \
        }                                                                                                              \
                                                                                                                       \
        StrIter saved = si;                                                                                            \
        while (StrIterPeek(&si, &c)) {                                                                                 \
            if (!is_valid_number_char(c, StrIterIndex(&si) == StrIterIndex(&saved), false)) {                          \
                break;                                                                                                 \
            }                                                                                                          \
            StrIterMustNext(&si);                                                                                      \
        }                                                                                                              \
                                                                                                                       \
        Zstr start = StrIterDataAt(&si, StrIterIndex(&saved));                                                         \
        size pos   = StrIterIndex(&si) - StrIterIndex(&saved);                                                         \
        Str  temp  = StrInitFromCstr(start, pos, &scratch);                                                            \
                                                                                                                       \
        if (StrLen(&temp) == 2 && StrBegin(&temp)[0] == '0' &&                                                         \
            (StrBegin(&temp)[1] == 'x' || StrBegin(&temp)[1] == 'X' || StrBegin(&temp)[1] == 'b' ||                    \
             StrBegin(&temp)[1] == 'B' || StrBegin(&temp)[1] == 'o' || StrBegin(&temp)[1] == 'O')) {                   \
            LOG_ERROR("Incomplete number format");                                                                     \
            StrDeinit(&temp);                                                                                          \
            DefaultAllocatorDeinit(&scratch);                                                                          \
            return start;                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        if (!is_valid_numeric_string(&temp, false)) {                                                                  \
            LOG_ERROR("Invalid numeric format");                                                                       \
            StrDeinit(&temp);                                                                                          \
            DefaultAllocatorDeinit(&scratch);                                                                          \
            return start;                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        VAL_T val;                                                                                                     \
        if (!PARSER(&temp, &val, NULL)) {                                                                              \
            LOG_ERROR("Failed to parse " #NAME);                                                                       \
            StrDeinit(&temp);                                                                                          \
            DefaultAllocatorDeinit(&scratch);                                                                          \
            return start;                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        BOUND_CHECK                                                                                                    \
                                                                                                                       \
        *v = (T)val;                                                                                                   \
        StrDeinit(&temp);                                                                                              \
        DefaultAllocatorDeinit(&scratch);                                                                              \
        return start + pos;                                                                                            \
    }

// Unsigned bound: log + cleanup + early return on val > UMAX.
#define _U_BOUND(NAME, UMAX)                                                                                           \
    if (val > UMAX) {                                                                                                  \
        LOG_ERROR("Value {} exceeds " #NAME " maximum ({})", val, UMAX);                                               \
        StrDeinit(&temp);                                                                                              \
        DefaultAllocatorDeinit(&scratch);                                                                              \
        return start;                                                                                                  \
    }

// Signed bound: log + cleanup + early return on val outside [IMIN, IMAX].
#define _I_BOUND(NAME, IMIN, IMAX)                                                                                     \
    if (val > IMAX || val < IMIN) {                                                                                    \
        LOG_ERROR("Value {} outside " #NAME " range ({} to {})", val, IMIN, IMAX);                                     \
        StrDeinit(&temp);                                                                                              \
        DefaultAllocatorDeinit(&scratch);                                                                              \
        return start;                                                                                                  \
    }

_MAKE_READ_TXT_INT(u16, u16, u64, StrToU64, _U_BOUND(u16, UINT16_MAX))
_MAKE_READ_TXT_INT(u32, u32, u64, StrToU64, _U_BOUND(u32, UINT32_MAX))
_MAKE_READ_TXT_INT(u64, u64, u64, StrToU64, /* val is already u64; no bound */)
_MAKE_READ_TXT_INT(i8, i8, i64, StrToI64, _I_BOUND(i8, INT8_MIN, INT8_MAX))
_MAKE_READ_TXT_INT(i16, i16, i64, StrToI64, _I_BOUND(i16, INT16_MIN, INT16_MAX))
_MAKE_READ_TXT_INT(i32, i32, i64, StrToI64, _I_BOUND(i32, INT32_MIN, INT32_MAX))
_MAKE_READ_TXT_INT(i64, i64, i64, StrToI64, /* val is already i64; no bound */)

#undef _MAKE_READ_TXT_INT
#undef _U_BOUND
#undef _I_BOUND

Zstr _read_Zstr(Zstr i, FmtInfo *fmt_info, Zstr *out) {
    (void)fmt_info;
    (void)out;
    LOG_FATAL("Zstr reads require explicit allocator provenance; use ZstrIO(zstr, alloc) instead");
    return i;
}

Zstr _read_ZstrAlloc(Zstr i, FmtInfo *fmt_info, ZstrIOArg *arg) {
    char     **out           = NULL;
    char      *previous      = NULL;
    char      *result        = NULL;
    Zstr       next          = NULL;
    Allocator *allocator_ptr = NULL;
    Str        temp;
    FmtInfo    default_fmt;

    if (!i || !arg || !arg->value || !arg->allocator) {
        LOG_FATAL("Invalid arguments");
    }

    out           = (char **)arg->value;
    allocator_ptr = arg->allocator;
    previous      = *out;
    temp          = StrInit(allocator_ptr);

    default_fmt        = fmt_info ? *fmt_info :
                                    (FmtInfo) {
                                        .align        = ALIGN_RIGHT,
                                        .width        = 0,
                                        .precision    = 6,
                                        .flags        = FMT_FLAG_NONE,
                                        .max_read_len = (u32)ZstrLen(i),
                             };
    default_fmt.flags &= ~FMT_FLAG_CHAR;
    if (!default_fmt.max_read_len) {
        default_fmt.max_read_len = (u32)ZstrLen(i);
    }

    next = _read_Str(i, &default_fmt, &temp);
    if (next == i) {
        StrDeinit(&temp);
        return i;
    }

    // Cast off const: the ZstrIO `out` parameter is `char **`, signalling
    // the caller owns and may mutate the returned buffer. ZstrDupN's Zstr
    // return is just the project-wide convention for fresh allocations.
    result = (char *)ZstrDupN(StrBegin(&temp), StrLen(&temp), allocator_ptr);
    if (!result) {
        LOG_ERROR("Failed to allocate memory for string");
        StrDeinit(&temp);
        return i;
    }

    if (previous) {
        AllocatorFree(allocator_ptr, previous);
    }

    *out = result;
    StrDeinit(&temp);
    return next;
}

#if FEATURE_BITVEC
bool _write_BitVec(Str *o, FmtInfo *fmt_info, BitVec *bv) {
    if (!o || !fmt_info || !bv) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    ValidateStr(o);
    ValidateBitVec(bv);

    // Snapshot the pre-render length so post-render padding sees the
    // size of just-this-field, not the accumulated buffer.
    size start_len = StrLen(o);

    if (fmt_info->flags & FMT_FLAG_HEX) {
        if (BitVecLen(bv) == 0) {
            // Render "0x0" / "0o0" placeholders for the zero-length
            // edge case so the prefix is visible even with no bits.
            if (!StrPushBackMany(o, "0x0")) {
                return false;
            }
        } else {
            // BitVecToInteger truncates at 64 bits -- this path renders
            // the bottom 64 bits as a regular integer; longer BitVecs
            // lose their upper bits in hex/octal display modes.
            u64          value  = BitVecToInteger(bv);
            StrIntFormat config = {.base = 16, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0, .use_prefix = true};
            if (!StrFromU64(o, value, &config)) {
                return false;
            }
        }
    } else if (fmt_info->flags & FMT_FLAG_OCTAL) {
        if (BitVecLen(bv) == 0) {
            if (!StrPushBackMany(o, "0o0")) {
                return false;
            }
        } else {
            u64          value  = BitVecToInteger(bv);
            StrIntFormat config = {.base = 8, .uppercase = false, .use_prefix = true};
            if (!StrFromU64(o, value, &config)) {
                return false;
            }
        }
    } else {
        // No flag -> render the raw 0/1 bit string (the BitVec's
        // native textual form).
        if (BitVecLen(bv) == 0) {
            // A zero-length BitVec renders empty; padding handles any
            // requested width below.
        } else {
            Str bit_str;

            if (!bitvec_try_to_str(&bit_str, bv, StrAllocator(o))) {
                return false;
            }
            if (!StrMerge(o, &bit_str)) {
                StrDeinit(&bit_str);
                return false;
            }
            StrDeinit(&bit_str);
        }
    }

    if (fmt_info->width > 0) {
        size content_len = StrLen(o) - start_len;
        if (!StrPad(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}
#endif // FEATURE_BITVEC

#if FEATURE_INT
bool _write_Int(Str *o, FmtInfo *fmt_info, Int *value) {
    if (!o || !fmt_info || !value) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    ValidateStr(o);
    ValidateInt(value);

    if (fmt_info->flags & FMT_FLAG_CHAR) {
        u64 byte_len = IntByteLength(value);

        if (byte_len == 0) {
            return true;
        }

        u8 *buffer = (u8 *)AllocatorAlloc(StrAllocator(o), byte_len * sizeof(u8), true);

        if (!buffer) {
            LOG_ERROR("Failed to allocate buffer for Int character formatting");
            return false;
        }

        (void)IntToBytesBE(value, buffer, byte_len);
        if (!write_char_internal(o, fmt_info->flags, (Zstr)buffer, byte_len)) {
            AllocatorFree(StrAllocator(o), buffer);
            return false;
        }
        AllocatorFree(StrAllocator(o), buffer);
        return true;
    }

    size start_len = StrLen(o);
    Str  temp;
    u8   radix = int_fmt_radix_from_flags(fmt_info);

    if (radix == 10) {
        if (!int_try_to_str(&temp, value, StrAllocator(o))) {
            return false;
        }
    } else {
        if (!int_try_to_str_radix(&temp, value, radix, (fmt_info->flags & FMT_FLAG_CAPS) != 0, StrAllocator(o))) {
            return false;
        }
    }

    if (!StrMerge(o, &temp)) {
        StrDeinit(&temp);
        return false;
    }
    StrDeinit(&temp);

    if (fmt_info->width > 0) {
        size content_len = StrLen(o) - start_len;
        if (!StrPad(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}
#endif // FEATURE_INT

#if FEATURE_BITVEC
Zstr _read_BitVec(Zstr i, FmtInfo *fmt_info, BitVec *bv) {
    (void)fmt_info; // Unused parameter
    if (!i || !bv) {
        LOG_FATAL("Invalid arguments");
        return i;
    }

    ValidateBitVec(bv);

    StrIter si = StrIterFromZstr(i);
    char    c  = 0;

    while (StrIterPeek(&si, &c) && IS_SPACE(c)) {
        StrIterMustNext(&si);
    }

    if (!StrIterRemainingLength(&si)) {
        LOG_ERROR("Empty input string");
        return StrIterDataAt(&si, StrIterIndex(&si));
    }

    Zstr start = StrIterDataAt(&si, StrIterIndex(&si));

    char c0 = 0;
    char c1 = 0;
    (void)StrIterPeekAt(&si, 0, &c0);
    (void)StrIterPeekAt(&si, 1, &c1);

    if (c0 == '0' && (c1 == 'x' || c1 == 'X')) {
        // Audit: precondition is the c0/c1 prefix check above. c1 starts
        // zero-initialised; the only way it carries 'x'/'X' here is that
        // the offset-1 peek succeeded, which proves 2 bytes are available.
        StrIterMustMove(&si, 2);
        StrIter hex_saved = si;

        while (StrIterPeek(&si, &c) && IS_XDIGIT(c)) {
            StrIterMustNext(&si);
        }

        if (StrIterIndex(&si) == StrIterIndex(&hex_saved)) {
            LOG_ERROR("Invalid hex format - no digits after 0x");
            return start;
        }

        Str hex_str = StrInitFromCstr(
            StrIterDataAt(&si, StrIterIndex(&hex_saved)),
            StrIterIndex(&si) - StrIterIndex(&hex_saved),
            BitVecAllocator(bv)
        );
        u64            value;
        StrParseConfig config = {.base = 16};
        if (!StrToU64(&hex_str, &value, &config)) {
            LOG_ERROR("Failed to parse hex value");
            StrDeinit(&hex_str);
            return start;
        }

        // The result BitVec needs at least one bit per encoded digit, so
        // hex pins it at 4, octal at 3 below -- otherwise round-tripping
        // a leading-zero literal would lose width information.
        u64 bit_len = value == 0 ? 1 : 64 - count_leading_zeros_u64(value);
        if (bit_len < 4)
            bit_len = 4;

        *bv = BitVecFromInteger(value, bit_len, BitVecAllocator(bv));
        StrDeinit(&hex_str);
        return StrIterDataAt(&si, StrIterIndex(&si));
    }

    if (c0 == '0' && (c1 == 'o' || c1 == 'O')) {
        // Audit: same proof as the hex branch above -- c1 carrying 'o'/'O'
        // can only happen when the offset-1 peek succeeded, proving 2 bytes
        // remain ahead of the cursor.
        StrIterMustMove(&si, 2);
        StrIter oct_saved = si;

        while (StrIterPeek(&si, &c) && c >= '0' && c <= '7') {
            StrIterMustNext(&si);
        }

        if (StrIterIndex(&si) == StrIterIndex(&oct_saved)) {
            LOG_ERROR("Invalid octal format - no digits after 0o");
            return start;
        }

        Str oct_str = StrInitFromCstr(
            StrIterDataAt(&si, StrIterIndex(&oct_saved)),
            StrIterIndex(&si) - StrIterIndex(&oct_saved),
            BitVecAllocator(bv)
        );
        u64            value;
        StrParseConfig config = {.base = 8};
        if (!StrToU64(&oct_str, &value, &config)) {
            LOG_ERROR("Failed to parse octal value");
            StrDeinit(&oct_str);
            return start;
        }

        u64 bit_len = value == 0 ? 1 : 64 - count_leading_zeros_u64(value);
        if (bit_len < 3)
            bit_len = 3;

        *bv = BitVecFromInteger(value, bit_len, BitVecAllocator(bv));
        StrDeinit(&oct_str);
        return StrIterDataAt(&si, StrIterIndex(&si));
    }

    // No `0x`/`0o` prefix: read the raw 0/1 bit string.
    StrIter bin_saved = si;

    while (StrIterPeek(&si, &c) && (c == '0' || c == '1')) {
        StrIterMustNext(&si);
    }

    if (StrIterIndex(&si) == StrIterIndex(&bin_saved)) {
        LOG_ERROR("Invalid binary format - expected 0s and 1s");
        return start;
    }

    Str bin_str = StrInitFromCstr(
        StrIterDataAt(&si, StrIterIndex(&bin_saved)),
        StrIterIndex(&si) - StrIterIndex(&bin_saved),
        BitVecAllocator(bv)
    );

    *bv = BitVecFromStr(StrBegin(&bin_str), BitVecAllocator(bv));

    StrDeinit(&bin_str);
    return StrIterDataAt(&si, StrIterIndex(&si));
}
#endif // FEATURE_BITVEC

#if FEATURE_INT
Zstr _read_Int(Zstr i, FmtInfo *fmt_info, Int *value) {
    if (!i || !value) {
        LOG_FATAL("Invalid arguments");
    }

    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        LOG_ERROR("Character-format reads are not supported for Int");
        return i;
    }

    ValidateInt(value);

    StrIter si = StrIterFromZstr(i);
    char    c  = 0;

    while (StrIterPeek(&si, &c) && IS_SPACE(c)) {
        StrIterMustNext(&si);
    }

    if (!StrIterRemainingLength(&si)) {
        LOG_ERROR("Failed to parse Int: empty input");
        return StrIterDataAt(&si, StrIterIndex(&si));
    }

    StrIter saved        = si;
    StrIter digits_saved = si;
    Zstr    start        = StrIterDataAt(&si, StrIterIndex(&saved));
    u8      radix        = int_fmt_radix_from_flags(fmt_info);

    char d0 = 0;
    (void)StrIterPeek(&si, &d0);
    if (d0 == '+') {
        StrIterMustNext(&si);
        digits_saved = si;
    }

    char p0 = 0;
    char p1 = 0;
    (void)StrIterPeekAt(&si, 0, &p0);
    (void)StrIterPeekAt(&si, 1, &p1);

    if (radix == 16 && p0 == '0' && (p1 == 'x' || p1 == 'X')) {
        LOG_ERROR("Int hex reads expect plain hex digits without a 0x prefix");
        return start;
    }
    if (radix == 2 && p0 == '0' && (p1 == 'b' || p1 == 'B')) {
        LOG_ERROR("Int binary reads expect plain binary digits without a 0b prefix");
        return start;
    }
    if (radix == 8 && p0 == '0' && (p1 == 'o' || p1 == 'O')) {
        LOG_ERROR("Int octal reads expect plain octal digits without a 0o prefix");
        return start;
    }

    while (StrIterPeek(&si, &c) && int_fmt_digit_matches_radix(c, radix)) {
        StrIterMustNext(&si);
    }

    if (StrIterIndex(&si) == StrIterIndex(&digits_saved)) {
        LOG_ERROR("Failed to parse Int");
        return start;
    }

    char trailing = 0;
    if (StrIterPeek(&si, &trailing) && trailing == '_') {
        LOG_ERROR("Int reads do not accept digit separators");
        return start;
    }

    Str  temp   = StrInitFromCstr(start, StrIterIndex(&si) - StrIterIndex(&saved), IntAllocator(value));
    Int  parsed = IntInit(IntAllocator(value));
    bool ok     = IntTryFromStrRadix(&parsed, StrBegin(&temp), radix);

    if (!ok) {
        StrDeinit(&temp);
        return start;
    }

    IntDeinit(value);
    *value = parsed;

    StrDeinit(&temp);
    return StrIterDataAt(&si, StrIterIndex(&si));
}
#endif // FEATURE_INT

#if FEATURE_FLOAT
Zstr _read_Float(Zstr i, FmtInfo *fmt_info, Float *value) {
    size  token_len = 0;
    Zstr  start     = NULL;
    Str   temp;
    Float parsed;

    if (!i || !value) {
        LOG_FATAL("Invalid arguments");
    }

    temp   = StrInit(FloatAllocator(value));
    parsed = FloatInit(FloatAllocator(value));

    if (float_fmt_uses_unsupported_flags(fmt_info)) {
        LOG_ERROR("Float only supports decimal and scientific reading");
        StrDeinit(&temp);
        FloatDeinit(&parsed);
        return i;
    }

    ValidateFloat(value);

    StrIter si = StrIterFromZstr(i);
    char    c  = 0;

    while (StrIterPeek(&si, &c) && IS_SPACE(c)) {
        StrIterMustNext(&si);
    }

    if (!StrIterRemainingLength(&si)) {
        LOG_ERROR("Failed to parse Float: empty input");
        StrDeinit(&temp);
        FloatDeinit(&parsed);
        return StrIterDataAt(&si, StrIterIndex(&si));
    }

    start     = StrIterDataAt(&si, StrIterIndex(&si));
    token_len = float_fmt_token_length(start);

    if (token_len == 0) {
        LOG_ERROR("Failed to parse Float");
        StrDeinit(&temp);
        FloatDeinit(&parsed);
        return start;
    }

    StrDeinit(&temp);
    temp = StrInitFromCstr(start, token_len, FloatAllocator(value));
    if (!FloatTryFromStr(&parsed, StrBegin(&temp))) {
        StrDeinit(&temp);
        FloatDeinit(&parsed);
        return start;
    }

    FloatDeinit(value);
    *value = parsed;

    StrDeinit(&temp);
    return start + token_len;
}
#endif // FEATURE_FLOAT

Zstr _read_f32(Zstr i, FmtInfo *fmt_info, f32 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        u32  temp = 0;
        Zstr next = read_chars_internal(i, (u8 *)&temp, sizeof(temp), fmt_info);
        *v        = (f32)temp;
        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    StrIter si = StrIterFromZstr(i);
    char    c  = 0;

    while (StrIterPeek(&si, &c) && IS_SPACE(c)) {
        StrIterMustNext(&si);
    }

    if (!StrIterRemainingLength(&si)) {
        LOG_ERROR("Failed to parse f32: empty input");
        DefaultAllocatorDeinit(&scratch);
        return StrIterDataAt(&si, StrIterIndex(&si));
    }

    // `inf` / `nan` / `-inf` / `-nan` need a wider token-scan than
    // the digit-by-digit number path; peek one ahead so a leading `-`
    // followed by `i`/`I` enters this branch too.
    char c1 = 0;
    (void)StrIterPeekAt(&si, 1, &c1);
    if ((c == 'i' || c == 'I' || c == 'n' || c == 'N') || (c == '-' && (c1 == 'i' || c1 == 'I'))) {
        StrIter saved = si;
        Zstr    start = StrIterDataAt(&si, StrIterIndex(&si));
        while (StrIterPeek(&si, &c) && !IS_SPACE(c)) {
            StrIterMustNext(&si);
        }

        Str temp = StrInitFromCstr(start, (size)(StrIterIndex(&si) - StrIterIndex(&saved)), &scratch);

        // Special-value tokens (`inf`, `nan`, ...) carry through the
        // generic Str-to-float path; non-matching tokens fall back to
        // the digit-by-digit scan below.
        f64 val;
        if (StrToF64(&temp, &val, NULL)) {
            *v = (f32)val;
            StrDeinit(&temp);
            DefaultAllocatorDeinit(&scratch);
            return StrIterDataAt(&si, StrIterIndex(&si));
        }
        StrDeinit(&temp);

        si = saved;
    }

    StrIter saved       = si;
    bool    has_decimal = false;

    while (StrIterPeek(&si, &c)) {
        if (c == '.') {
            if (has_decimal)
                break;
            has_decimal = true;
        }

        // Exponent introducer: consume the optional sign and require at
        // least one digit, otherwise rewind so the 'e' is left to the
        // caller's tail.
        if ((c == 'e' || c == 'E') && StrIterIndex(&si) > StrIterIndex(&saved)) {
            StrIterMustNext(&si);

            if (StrIterPeek(&si, &c) && (c == '+' || c == '-')) {
                StrIterMustNext(&si);
            }

            if (!StrIterPeek(&si, &c) || !IS_DIGIT(c)) {
                StrIterMustPrev(&si);
                break;
            }

            while (StrIterPeek(&si, &c) && IS_DIGIT(c)) {
                StrIterMustNext(&si);
            }
            break;
        }

        if (!is_valid_number_char(c, StrIterIndex(&si) == StrIterIndex(&saved), true)) {
            break;
        }

        StrIterMustNext(&si);
    }

    Zstr start = StrIterDataAt(&si, StrIterIndex(&saved));
    size pos   = StrIterIndex(&si) - StrIterIndex(&saved);

    Str temp = StrInitFromCstr(start, pos, &scratch);

    if (!is_valid_numeric_string(&temp, true)) {
        LOG_ERROR("Invalid floating point format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Parse through the f64 path and narrow at the end; an explicit
    // f32 parser would duplicate the entire StrToF64 logic for no gain.
    f64 val;
    if (!StrToF64(&temp, &val, NULL)) {
        LOG_ERROR("Failed to parse f32");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    *v = (f32)val;
    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

static bool _write_r8(Str *o, FmtInfo *fmt_info, u8 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    return StrPushBackR(o, *v);
}

static bool _write_r16(Str *o, FmtInfo *fmt_info, u16 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }

    u16 x = *v;
    switch (fmt_info->endian) {
        case ENDIAN_BIG : {
            return StrPushBackR(o, ((x >> 8) & 0xff)) && StrPushBackR(o, (x & 0xff));
        }
        case ENDIAN_LITTLE : {
            return StrPushBackR(o, (x & 0xff)) && StrPushBackR(o, ((x >> 8) & 0xff));
        }
        case ENDIAN_NATIVE :
        default : {
            LOG_FATAL("Invalid endianness");
            return false;
        }
    }
}

static bool _write_r32(Str *o, FmtInfo *fmt_info, u32 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }

    u32 x = *v;
    switch (fmt_info->endian) {
        case ENDIAN_BIG : {
            return StrPushBackR(o, ((x >> 24) & 0xff)) && StrPushBackR(o, (x >> 16) & 0xff) &&
                   StrPushBackR(o, (x >> 8) & 0xff) && StrPushBackR(o, (x & 0xff));
        }
        case ENDIAN_LITTLE : {
            return StrPushBackR(o, (x & 0xff)) && StrPushBackR(o, (x >> 8) & 0xff) &&
                   StrPushBackR(o, (x >> 16) & 0xff) && StrPushBackR(o, ((x >> 24) & 0xff));
        }
        case ENDIAN_NATIVE :
        default : {
            LOG_FATAL("Invalid endianness");
            return false;
        }
    }
}

static bool _write_r64(Str *o, FmtInfo *fmt_info, u64 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }

    u64 x = *v;
    switch (fmt_info->endian) {
        case ENDIAN_BIG : {
            return StrPushBackR(o, ((x >> 56) & 0xff)) && StrPushBackR(o, (x >> 48) & 0xff) &&
                   StrPushBackR(o, (x >> 40) & 0xff) && StrPushBackR(o, (x >> 32) & 0xff) &&
                   StrPushBackR(o, (x >> 24) & 0xff) && StrPushBackR(o, (x >> 16) & 0xff) &&
                   StrPushBackR(o, (x >> 8) & 0xff) && StrPushBackR(o, (x & 0xff));
        }
        case ENDIAN_LITTLE : {
            return StrPushBackR(o, (x & 0xff)) && StrPushBackR(o, (x >> 8) & 0xff) &&
                   StrPushBackR(o, (x >> 16) & 0xff) && StrPushBackR(o, (x >> 24) & 0xff) &&
                   StrPushBackR(o, (x >> 32) & 0xff) && StrPushBackR(o, (x >> 40) & 0xff) &&
                   StrPushBackR(o, (x >> 48) & 0xff) && StrPushBackR(o, ((x >> 56) & 0xff));
        }
        case ENDIAN_NATIVE :
        default : {
            LOG_FATAL("Invalid endianness");
            return false;
        }
    }
}

static Zstr _read_r8(Zstr i, FmtInfo *fmt_info, u8 *v) {
    if (!i || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
    }

    *v = (u8)*i;
    return i + 1;
}

static Zstr _read_r16(Zstr i, FmtInfo *fmt_info, u16 *v) {
    if (!i || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
    }

    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }

    // Read as u8 (not signed char) to avoid sign extension on platforms
    // where `char` is signed.
    const u8 *p = (const u8 *)i;

    switch (fmt_info->endian) {
        case ENDIAN_BIG :
            *v = ((u16)p[0] << 8) | p[1];
            break;
        case ENDIAN_LITTLE :
            *v = ((u16)p[1] << 8) | p[0];
            break;
        case ENDIAN_NATIVE :
        default :
            LOG_FATAL("Invalid endianness");
    }

    return i + 2; // Advance the stream pointer by 2 bytes.
}

static Zstr _read_r32(Zstr i, FmtInfo *fmt_info, u32 *v) {
    if (!i || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
    }

    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }

    const u8 *p = (const u8 *)i;

    switch (fmt_info->endian) {
        case ENDIAN_BIG :
            *v = ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
            break;
        case ENDIAN_LITTLE :
            *v = ((u32)p[3] << 24) | ((u32)p[2] << 16) | ((u32)p[1] << 8) | p[0];
            break;
        case ENDIAN_NATIVE :
        default :
            LOG_FATAL("Invalid endianness");
    }

    return i + 4; // Advance the stream pointer by 4 bytes.
}

static Zstr _read_r64(Zstr i, FmtInfo *fmt_info, u64 *v) {
    if (!i || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
    }

    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }

    const u8 *p = (const u8 *)i;

    switch (fmt_info->endian) {
        case ENDIAN_BIG :
            *v = ((u64)p[0] << 56) | ((u64)p[1] << 48) | ((u64)p[2] << 40) | ((u64)p[3] << 32) | ((u64)p[4] << 24) |
                 ((u64)p[5] << 16) | ((u64)p[6] << 8) | (u64)p[7];
            break;
        case ENDIAN_LITTLE :
            *v = ((u64)p[7] << 56) | ((u64)p[6] << 48) | ((u64)p[5] << 40) | ((u64)p[4] << 32) | ((u64)p[3] << 24) |
                 ((u64)p[2] << 16) | ((u64)p[1] << 8) | (u64)p[0];
            break;
        case ENDIAN_NATIVE :
        default :
            LOG_FATAL("Invalid endianness");
    }

    return i + 8; // Advance the stream pointer by 8 bytes.
}
