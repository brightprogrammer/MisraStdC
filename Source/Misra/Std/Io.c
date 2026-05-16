/// file      : std/io.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// formatted reading/writing and other magical stuff

// Required for fileno
// Reference : https://forums.freebsd.org/threads/strerror_r-best-practices-posix-vs-gnu.92296/
#define _POSIX_C_SOURCE 200112L

#if defined(_WIN32)
#    include <io.h>
#    define FILENO _fileno
#else
#    include <unistd.h>
#    define FILENO fileno
#endif

#include "../_Syscall.h"

// Returns 1 if `fd` is a TTY, 0 otherwise. Linux: direct ioctl(TCGETS).
// macOS/BSD: libSystem isatty. Windows: CRT _isatty.
static inline int misra_is_tty(int fd) {
#if defined(_WIN32)
    return _isatty(fd);
#elif FEATURE_DIRECT_SYSCALL
    char buf[128]; // termios is < 64 bytes; 128 is safe overkill.
    long ret = misra_sys3(MISRA_SYS_ioctl, (long)fd, 0x5401L, (long)(uintptr_t)buf);
    return ret == 0 ? 1 : 0;
#else
    return isatty(fd);
#endif
}

#define ISATTY misra_is_tty

#ifndef STDIN_FILENO
#    define STDIN_FILENO FILENO(stdin)
#endif
#ifndef STDOUT_FILENO
#    define STDOUT_FILENO FILENO(stdout)
#endif
#ifndef STDERR_FILENO
#    define STDERR_FILENO FILENO(stderr)
#endif

#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>
#include <Misra/Std/Memory.h>
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

// stdc
#include <math.h>
#include <stdio.h>


// Tiny in-tree hex helpers so Io doesn't need libc isxdigit/strtol.
// Returns -1 on a non-hex digit, otherwise 0..15.
static inline int hex_nibble(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

// Parses two hex chars into a byte; returns -1 if either is not hex.
static inline int hex_byte(char hi, char lo) {
    int h = hex_nibble(hi);
    int l = hex_nibble(lo);
    if (h < 0 || l < 0)
        return -1;
    return (h << 4) | l;
}

static bool _write_r8(Str *o, FmtInfo *fmt_info, u8 *v);
static bool _write_r16(Str *o, FmtInfo *fmt_info, u16 *v);
static bool _write_r32(Str *o, FmtInfo *fmt_info, u32 *v);
static bool _write_r64(Str *o, FmtInfo *fmt_info, u64 *v);

static const char *_read_r8(const char *i, FmtInfo *fmt_info, u8 *v);
static const char *_read_r16(const char *i, FmtInfo *fmt_info, u16 *v);
static const char *_read_r32(const char *i, FmtInfo *fmt_info, u32 *v);
static const char *_read_r64(const char *i, FmtInfo *fmt_info, u64 *v);

// Portable helper for counting leading zeros in a 64-bit integer
static inline u64 count_leading_zeros_u64(u64 value) {
    if (value == 0)
        return 64;

    // Pure portable implementation - works on all compilers
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

// Helper function to parse format specifiers
// {[(alignment/endianness)[alignment-width/raw-read-width]](specifier)}
static bool ParseFormatSpec(const char *spec, u32 len, FmtInfo *fi) {
    if (!spec || !fi) {
        LOG_FATAL("Invalid arguments to ParseFormatSpec");
        return false;
    }
    // Empty format specifier is allowed, but spec pointer must not be NULL

    // Initialize format info with defaults
    *fi = (FmtInfo) {.align = ALIGN_RIGHT, .width = 0, .precision = 6, .flags = FMT_FLAG_NONE};

    u32 pos = 0;

    // Parse alignment
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

    // Parse width
    u32 width = 0;
    if (pos < len && spec[pos] >= '0' && spec[pos] <= '9') {
        while (pos < len && spec[pos] >= '0' && spec[pos] <= '9') {
            width = width * 10 + (spec[pos] - '0');
            pos++;
        }
    }
    fi->width = width;

    // Parse precision only if this is not a raw field
    if (pos < len && spec[pos] == '.') {
        pos++;
        if (pos >= len || spec[pos] < '0' || spec[pos] > '9') {
            return false; // Precision must be followed by digits
        }
        size precision = 0;
        while (pos < len && spec[pos] >= '0' && spec[pos] <= '9') {
            precision = precision * 10 + (spec[pos] - '0');
            pos++;
        }
        fi->flags     |= FMT_FLAG_HAS_PRECISION;
        fi->precision  = precision;
    }

    // Parse format type after precision
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

// Helper function to pad string with spaces
static bool PadString(Str *o, size width, Alignment align, size content_len) {
    if (content_len >= width)
        return true;

    size pad_len = width - content_len;

    if (align == ALIGN_RIGHT) {
        // Pad on left
        for (size i = 0; i < pad_len; i++) {
            if (!StrPushFront(o, ' ')) {
                return false;
            }
        }
    } else if (align == ALIGN_LEFT) {
        // Pad on right
        for (size i = 0; i < pad_len; i++) {
            if (!StrPushBack(o, ' ')) {
                return false;
            }
        }
    } else { // ALIGN_CENTER
        size left_pad  = pad_len / 2;
        size right_pad = pad_len - left_pad;

        // Pad on left
        for (size i = 0; i < left_pad; i++) {
            if (!StrPushFront(o, ' ')) {
                return false;
            }
        }

        // Pad on right
        for (size i = 0; i < right_pad; i++) {
            if (!StrPushBack(o, ' ')) {
                return false;
            }
        }
    }

    return true;
}

bool str_write_fmt(Str *o, const char *fmt, TypeSpecificIO *args, u64 argc) {
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
            // Check for escaped brace
            if (i + 1 < fmt_len && fmt[i + 1] == '{') {
                if (!StrPushBack(o, '{')) {
                    return false;
                }
                i++; // Skip next brace
                continue;
            }

            // Find closing brace
            size brace_start = i;
            size brace_end   = i + 1;
            while (brace_end < fmt_len && fmt[brace_end] != '}') {
                brace_end++;
            }

            // Error if no closing brace found
            if (brace_end >= fmt_len) {
                LOG_ERROR("Unclosed format specifier");
                return false;
            }

            // Extract format specifier
            size spec_len = brace_end - brace_start - 1;

            // Parse format specifier
            FmtInfo fmt_info;
            if (spec_len == 0) {
                // Empty format specifier {} is allowed, initialize with defaults
                fmt_info = (FmtInfo) {.align = ALIGN_RIGHT, .width = 0, .precision = 6, .flags = FMT_FLAG_NONE};
            } else if (!ParseFormatSpec(fmt + brace_start + 1, spec_len, &fmt_info)) {
                return false;
            }

            // Check if we have enough arguments
            if (arg_idx >= argc) {
                LOG_ERROR("Not enough arguments for format string");
                return false;
            }

            // Get current argument
            TypeSpecificIO *arg = &args[arg_idx++];
            if (!arg->writer || !arg->data) {
#if defined(_MSC_VER) || defined(__MSC_VER)
                LOG_INFO("Using default writer for char, because MSVC is STUPID AF");
                if (fmt_info.flags & FMT_FLAG_CHAR) {
                    arg->writer = (TypeSpecificWriter)_write_i8;
                } else {
                    arg->writer = (TypeSpecificWriter)_write_u64;
                }
#else
                LOG_FATAL("Invalid writer or data pointer");
#endif
            }

            // write raw if flag provided, otherwise switch to default writing
            if (fmt_info.flags & FMT_FLAG_RAW) {
                TypeSpecificWriter write_fn = arg->writer;

                // deduce the actual field size
                u32 var_width = 0;
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

                // get data in a variable of max width
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

                // write requested bytes
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
                // Write the formatted value
                if (!arg->writer(o, &fmt_info, arg->data)) {
                    return false;
                }
            }


            // Skip to end of format specifier
            i = brace_end;
        } else if (fmt[i] == '}') {
            // Check for escaped brace
            if (i + 1 < fmt_len && fmt[i + 1] == '}') {
                if (!StrPushBack(o, '}')) {
                    return false;
                }
                i++; // Skip next brace
                continue;
            }
            LOG_ERROR("Unmatched closing brace");
            return false;
        } else {
            if (!StrPushBack(o, fmt[i])) {
                return false;
            }
        }
    }

    // Check if we used all arguments
    if (arg_idx < argc) {
        LOG_ERROR("Too many arguments for format string");
        return false;
    }

    return true;
}

bool f_write_fmt(File *stream, const char *fmtstr, TypeSpecificIO *argv, u64 argc, bool append_newline) {
    Str              out;
    bool             ok      = true;
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!stream || !fmtstr) {
        LOG_FATAL("Invalid arguments");
        DefaultAllocatorDeinit(&scratch);
        return false;
    }

    out = StrInit(&scratch);
    ok  = str_write_fmt(&out, fmtstr, argv, argc);

    // Build the whole line first, including any trailing newline, then
    // emit in a single FileWrite. Single-syscall writes under PIPE_BUF
    // (4096) are atomic on POSIX, so concurrent threads don't shred
    // each other's output.
    if (ok && append_newline) {
        StrPushBack(&out, '\n');
    }

    if (ok && out.length > 0 && FileWrite(stream, out.data, out.length) != (i64)out.length) {
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

const char *str_read_fmt(const char *input, const char *fmtstr, TypeSpecificIO *argv, u64 argc) {
    if (!input || !fmtstr) {
        LOG_FATAL("Invalid arguments");
    }

    const char *p         = fmtstr;
    const char *in        = input;
    u64         rem_p     = ZstrLen(fmtstr);
    u64         rem_in    = ZstrLen(in);
    u64         arg_index = 0; // Current argument index

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

            // Find closing brace
            const char *start    = p;
            size        spec_len = 0;
            while (rem_p > 0 && *p != '}') {
                p++;
                rem_p--;
                spec_len++;
            }

            if (rem_p == 0 || *p != '}') {
                LOG_ERROR("Unmatched '{' in format string");
                return NULL;
            }

            // Parse optional specifier
            char spec_buf[32] = {0};
            if (spec_len >= sizeof(spec_buf)) {
                LOG_ERROR("Format specifier too long");
                return NULL;
            }

            if (arg_index >= argc) {
                LOG_ERROR("More placeholders than arguments");
                return NULL;
            }

            MemCopy(spec_buf, start, spec_len);
            spec_buf[spec_len] = '\0';

            // Validate format specifier
            FmtInfo fmt_info = {0};
            if (!ParseFormatSpec(spec_buf, spec_len, &fmt_info)) {
                LOG_ERROR("Failed to parse format specifier");
                return NULL; // Error already logged by ParseFormatSpec
            }
            fmt_info.max_read_len = rem_in;

            // Skip closing '}'
            p++;
            rem_p--;

            // Use the type-specific reader
            TypeSpecificIO *io = &argv[arg_index++];
            if (!io->reader) {
                LOG_ERROR("Missing reader function");
                return NULL;
            }

            // If raw data reading is specified, use raw readers
            const char        *next       = NULL;
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

                // do raw read
                u64 x = 0;
                next  = raw_reader(in, &fmt_info, &x);

                if (next) {
                    rem_in -= (next - in);
                }

                // deduce the actual field size
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

                // make sure we write only as much space is provided
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
                // find length between two format specifiers
                u64  space_len = 0;
                char c         = p[space_len];
                while (c) {
                    // if this is possible start of a new format specifier and not '{{' (escaped brace)
                    if (c == '{' && p[space_len + 1] != '{') {
                        break;
                    } else {
                        space_len++;
                    }
                    c = p[space_len];
                }

                // if there's something between two format specifiers then we can read only upto that
                // otherwise end of input is the limit
                if (space_len) {
                    // Find first occurence of content between current and next format specifier or null character
                    const char *e = NULL;
                    if ((e = ZstrFindSubstringN(in, p, space_len))) {
                        fmt_info.max_read_len = e - in;
                    }
                } else {
                    fmt_info.max_read_len = rem_in;
                }

                // do formatted read
                next = io->reader(in, &fmt_info, io->data);

                if (next) {
                    rem_in -= (next - in);
                }
            }

            // Check if reading failed
            if (!next || next == in) {
                LOG_ERROR("Failed to read value for placeholder {}", LVAL(arg_index - 1));
                return NULL;
            }

            // Update input pointer
            in = next;
        } else {
            // Match exact character from format string
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

void f_read_fmt(File *file, const char *fmtstr, TypeSpecificIO *argv, u64 argc) {
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
            StrPushBack(&buffer, buf_byte);
        }
        str_read_fmt(buffer.data, fmtstr, argv, argc);
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
            i64 got = FileRead(file, buffer.data, (u64)file_len);
            if (got < 0) {
                LOG_ERROR("FileRead failed during f_read_fmt");
                StrDeinit(&buffer);
                DefaultAllocatorDeinit(&scratch);
                return;
            }
            buffer.length = (u64)got;
        }

        if (buffer.length) {
            const char *new_pos = str_read_fmt(buffer.data, fmtstr, argv, argc);
            if (!new_pos) {
                LOG_ERROR("Parse failed, rolling back...");
                (void)FileSeek(file, cur_pos, FILE_SEEK_SET);
            } else {
                // Advance the channel position past the bytes we consumed.
                i64 consumed = (i64)(new_pos - buffer.data);
                (void)FileSeek(file, cur_pos + consumed, FILE_SEEK_SET);
            }
        }
    }

    StrDeinit(&buffer);
    DefaultAllocatorDeinit(&scratch);
}

// Helper function to write integer values as character sequences in a consistent order
// regardless of system endianness (big-endian order: most significant byte first)
static inline bool write_int_as_chars(Str *o, FormatFlags flags, u64 value, size num_bytes) {
    if (!o || !num_bytes || num_bytes > 8) {
        LOG_FATAL("Invalid arguments to write_int_as_chars");
    }

    bool is_caps    = (flags & FMT_FLAG_CAPS) != 0;
    bool force_case = (flags & FMT_FLAG_FORCE_CASE) != 0;

    // Process bytes in big-endian order (most significant byte first)
    for (size i = 0; i < num_bytes; i++) {
        // Extract byte at position (num_bytes - 1 - i) from the right
        u8 byte = (value >> ((num_bytes - 1 - i) * 8)) & 0xFF;

        if (IS_PRINTABLE(byte)) {
            // For 'a'/'A' format specifier (force_case), apply case conversion
            // For 'c' format specifier, preserve the original case
            if (force_case) {
                if (!StrPushBack(o, is_caps ? TO_UPPER(byte) : TO_LOWER(byte))) {
                    return false;
                }
            } else {
                if (!StrPushBack(o, byte)) {
                    return false;
                }
            }
        } else {
            // Handle non-printable characters
            u8   low = byte & 0xf;
            u8   hiw = (byte >> 4) & 0xf;
            char c1  = hiw < 10 ? '0' + hiw : is_caps ? 'A' + (hiw - 10) : 'a' + (hiw - 10);
            char c2  = low < 10 ? '0' + low : is_caps ? 'A' + (low - 10) : 'a' + (low - 10);

            if (!StrPushBackZstr(o, "\\x") || !StrPushBack(o, c1) || !StrPushBack(o, c2)) {
                return false;
            }
        }
    }

    return true;
}

static inline bool write_char_internal(Str *o, FormatFlags flags, const char *vs, size len) {
    if (!o || !vs || !len) {
        LOG_FATAL("Invalid arguments");
    }

    bool is_caps    = (flags & FMT_FLAG_CAPS) != 0;
    bool force_case = (flags & FMT_FLAG_FORCE_CASE) != 0;

    while (len--) {
        if (IS_PRINTABLE(*vs)) {
            // For 'a'/'A' format specifier (force_case), apply case conversion
            // For 'c' format specifier, preserve the original case
            if (force_case) {
                if (!StrPushBack(o, is_caps ? TO_UPPER(*vs) : TO_LOWER(*vs))) {
                    return false;
                }
            } else {
                if (!StrPushBack(o, *vs)) {
                    return false;
                }
            }
        } else {
            if (!StrPushBackZstr(o, "\\x")) {
                return false;
            }
            u8   c     = *vs;
            u8   low   = c & 0xf;
            u8   hiw   = (c >> 4) & 0xf;
            char c1    = hiw < 10 ? '0' + hiw : is_caps ? 'A' + (hiw - 10) : 'a' + (hiw - 10);
            char c2    = low < 10 ? '0' + low : is_caps ? 'A' + (low - 10) : 'a' + (low - 10);
            char cs[3] = {'\\', c1, c2};
            if (!StrPushBackCstr(o, cs, 3)) {
                return false;
            }
        }
        vs++;
    }

    return true;
}

#if FEATURE_INT
static int IntFmtDigitValue(char c) {
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

static bool IntFmtDigitMatchesRadix(char c, u8 radix) {
    int digit = IntFmtDigitValue(c);

    return digit >= 0 && digit < radix;
}

static u8 IntFmtRadixFromFlags(FmtInfo *fmt_info) {
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
static bool FloatFmtUsesUnsupportedFlags(FmtInfo *fmt_info) {
    return fmt_info && (fmt_info->flags & (FMT_FLAG_CHAR | FMT_FLAG_HEX | FMT_FLAG_BINARY | FMT_FLAG_OCTAL |
                                           FMT_FLAG_RAW | FMT_FLAG_STRING)) != 0;
}

static bool FloatFmtAppendExponent(Str *out, i64 exponent, bool uppercase) {
    char sign        = exponent < 0 ? '-' : '+';
    u64  magnitude   = exponent < 0 ? (u64)(-(exponent + 1)) + 1 : (u64)exponent;
    char digits[32]  = {0};
    u32  digit_count = 0;

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    if (magnitude == 0) {
        digits[digit_count++] = '0';
    } else {
        while (magnitude > 0) {
            digits[digit_count++]  = (char)('0' + (magnitude % 10));
            magnitude             /= 10;
        }
    }

    if (!StrPushBack(out, uppercase ? 'E' : 'e') || !StrPushBack(out, sign)) {
        return false;
    }

    if (digit_count < 2) {
        if (!StrPushBack(out, '0')) {
            return false;
        }
    }

    while (digit_count > 0) {
        if (!StrPushBack(out, digits[--digit_count])) {
            return false;
        }
    }

    return true;
}

static bool FloatFmtTryToDecimalStr(Str *out, Float *value, u32 precision, bool has_precision, Allocator *alloc) {
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
        const char *body   = canonical.data;
        const char *dot    = NULL;
        u64         prefix = 0;
        u64         frac   = 0;

        result = StrInit(alloc);

        if (canonical.data[0] == '-') {
            if (!StrPushBack(&result, '-')) {
                goto fail;
            }
            body++;
        }

        dot = ZstrFindChar(body, '.');
        if (!dot) {
            if (!StrPushBackCstr(&result, body, ZstrLen(body))) {
                goto fail;
            }

            if (precision > 0) {
                if (!StrPushBack(&result, '.')) {
                    goto fail;
                }
                for (u32 i = 0; i < precision; i++) {
                    if (!StrPushBack(&result, '0')) {
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

        if (!StrPushBackCstr(&result, body, prefix)) {
            goto fail;
        }
        if (precision > 0) {
            if (!StrPushBack(&result, '.')) {
                goto fail;
            }
            if (!StrPushBackCstr(&result, dot + 1, MIN2(frac, (u64)precision))) {
                goto fail;
            }
            for (u32 i = (u32)MIN2(frac, (u64)precision); i < precision; i++) {
                if (!StrPushBack(&result, '0')) {
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

static bool FloatFmtTryToScientificStr(
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
            if (!StrPushBack(&result, '-')) {
                goto fail;
            }
        }
        if (!StrPushBack(&result, '0')) {
            goto fail;
        }
        if (has_precision && precision > 0) {
            if (!StrPushBack(&result, '.')) {
                goto fail;
            }
            for (u32 i = 0; i < precision; i++) {
                if (!StrPushBack(&result, '0')) {
                    goto fail;
                }
            }
        }
        if (!FloatFmtAppendExponent(&result, 0, uppercase)) {
            goto fail;
        }
        StrDeinit(&digits);
        *out = result;
        return true;
    }

    if (value->negative) {
        if (!StrPushBack(&result, '-')) {
            goto fail;
        }
    }

    exponent = value->exponent + (i64)digits.length - 1;
    if (!StrPushBack(&result, digits.data[0])) {
        goto fail;
    }

    frac_digits = has_precision ? precision : (digits.length > 0 ? digits.length - 1 : 0);
    if (frac_digits > 0) {
        if (!StrPushBack(&result, '.')) {
            goto fail;
        }
        for (u64 i = 0; i < frac_digits; i++) {
            if (i + 1 < digits.length) {
                if (!StrPushBack(&result, digits.data[i + 1])) {
                    goto fail;
                }
            } else {
                if (!StrPushBack(&result, '0')) {
                    goto fail;
                }
            }
        }
    }

    if (!FloatFmtAppendExponent(&result, exponent, uppercase)) {
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

static size FloatFmtTokenLength(const char *input) {
    size pos            = 0;
    bool saw_digit      = false;
    bool saw_decimal    = false;
    bool saw_exponent   = false;
    bool need_exp_digit = false;
    bool allow_sign     = true;

    if (!input) {
        LOG_FATAL("input is NULL");
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

///
/// Helper function to read characters into a buffer, handling hex escape sequences
///
/// i[in]           : Pointer to current read position in input string
/// buffer[out]     : Buffer to store read characters
/// buffer_size[in] : Maximum number of characters to read into buffer
///
/// Returns: Updated pointer to position after reading characters
///
/// TAGS: Helper, Character, Reading, EscapeSequences
///
static inline const char *read_chars_internal(const char *i, u8 *buffer, size buffer_size, FmtInfo *fmt_info) {
    if (!i || !buffer || !buffer_size) {
        LOG_FATAL("Invalid arguments to read_chars_internal");
    }

    size        bytes_read = 0;
    const char *current    = i;
    bool        force_case = fmt_info && (fmt_info->flags & FMT_FLAG_FORCE_CASE) != 0;
    bool        is_caps    = fmt_info && (fmt_info->flags & FMT_FLAG_CAPS) != 0;

    while (bytes_read < buffer_size && *current && !IS_SPACE(*current)) {
        u8 char_to_store;

        if (current[0] == '\\' && current[1] == 'x') {
            // Handle hex escape sequence \xNN
            int hex_val = hex_byte(current[2], current[3]);
            if (hex_val >= 0) {
                char_to_store  = (u8)hex_val;
                current       += 4; // Skip \xNN
            } else {
                // Invalid hex sequence, treat as regular character
                char_to_store = (u8)*current;
                current++;
            }
        } else {
            // Regular character
            char_to_store = (u8)*current;
            current++;
        }

        // Apply case conversion if needed
        if (force_case) {
            char_to_store = is_caps ? TO_UPPER(char_to_store) : TO_LOWER(char_to_store);
        }

        buffer[bytes_read] = char_to_store;
        bytes_read++;
    }

    return current;
}

bool _write_Str(Str *o, FmtInfo *fmt_info, Str *s) {
    if (!o || !s || !fmt_info) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateStr(o);
    ValidateStr(s);

    // Store original length to calculate content size later
    size start_len = o->length;

    // Handle empty string - don't need special handling, just apply padding if needed
    if (s->length) {
        if (fmt_info->flags & FMT_FLAG_HEX) {
            // Format each character as hex
            StrIntFormat config = {.base = 16, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0};
            StrForeachIdx(s, c, i) {
                if (i > 0) {
                    if (!StrPushBack(o, ' ')) {
                        return false;
                    }
                }
                // Create hex string for each character
                Str hex = StrInit(o->allocator);
                if (!StrFromU64(&hex, c, &config)) {
                    StrDeinit(&hex);
                    return false;
                }
                // Ensure 2 digits with leading zero
                if (hex.length == 1) {
                    if (!StrPushFront(&hex, '0')) {
                        StrDeinit(&hex);
                        return false;
                    }
                }
                if (!StrPushBackZstr(o, "0x") || !StrMerge(o, &hex)) {
                    StrDeinit(&hex);
                    return false;
                }
                StrDeinit(&hex);
            }
        } else {
            // If precision is specified, use it as max length
            size len = s->length;
            if (fmt_info->flags & FMT_FLAG_HAS_PRECISION) {
                // Precision 0 means empty string (not an error)
                if (fmt_info->precision == 0) {
                    len = 0;
                } else {
                    len = MIN2(len, fmt_info->precision);
                }
            }

            // Copy string content
            if (fmt_info->flags & FMT_FLAG_CHAR) {
                if (!write_char_internal(o, fmt_info->flags, (const char *)s->data, len)) {
                    return false;
                }
            } else {
                StrForeachInRange(s, c, 0, len) {
                    if (IS_PRINTABLE(c)) {
                        if (!StrPushBack(o, c)) {
                            return false;
                        }
                    } else {
                        const char *digits = "0123456789abcdef";
                        if (!StrPushBackZstr(o, "\\x") || !StrPushBack(o, digits[(c >> 4) & 0xf]) ||
                            !StrPushBack(o, digits[c & 0xf])) {
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size content_len = o->length - start_len;
        if (!PadString(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}

bool _write_Zstr(Str *o, FmtInfo *fmt_info, const char **s) {
    if (!o || !s || !*s || !fmt_info) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    ValidateStr(o);

    // Store original length to calculate content size later
    size        start_len = o->length;
    const char *xs        = *s;

    // Handle null or empty string
    if (xs[0] != '\0') {
        if (fmt_info->flags & FMT_FLAG_HEX) {
            // Format each character as hex
            size i = 0;
            while (xs[i]) {
                if (i > 0) {
                    if (!StrPushBack(o, ' ')) {
                        return false;
                    }
                }
                // Create hex string for each character
                Str          hex    = StrInit(o->allocator);
                StrIntFormat config = {.base = 16, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0};
                if (!StrFromU64(&hex, (u8)xs[i], &config)) {
                    StrDeinit(&hex);
                    return false;
                }
                // Ensure 2 digits with leading zero
                if (hex.length == 1) {
                    if (!StrPushFront(&hex, '0')) {
                        StrDeinit(&hex);
                        return false;
                    }
                }
                if (!StrPushBackZstr(o, "0x") || !StrMerge(o, &hex)) {
                    StrDeinit(&hex);
                    return false;
                }
                StrDeinit(&hex);
                i++;
            }
        } else {
            // Get string length
            size len = ZstrLen(xs);

            // If precision is specified, use it as max length
            if (fmt_info->flags & FMT_FLAG_HAS_PRECISION) {
                // Precision 0 means empty string (not an error)
                if (fmt_info->precision == 0) {
                    len = 0;
                } else {
                    len = MIN2(len, fmt_info->precision);
                }
            }

            // Copy string content
            if (fmt_info->flags & FMT_FLAG_CHAR) {
                if (!write_char_internal(o, fmt_info->flags, xs, len)) {
                    return false;
                }
            } else {
                for (size i = 0; i < len; i++) {
                    if (IS_PRINTABLE(xs[i])) {
                        if (!StrPushBack(o, xs[i])) {
                            return false;
                        }
                    } else {
                        const char *digits = "0123456789abcdef";
                        if (!StrPushBackZstr(o, "\\x") || !StrPushBack(o, digits[(xs[i] >> 4) & 0xf]) ||
                            !StrPushBack(o, digits[xs[i] & 0xf])) {
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size content_len = o->length - start_len;
        if (!PadString(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}

bool _write_ZstrAlloc(Str *o, FmtInfo *fmt_info, ZstrIOArg *arg) {
    const char **value = NULL;

    if (!arg) {
        LOG_FATAL("Invalid arguments");
    }

    value = (const char **)arg->value;
    return _write_Zstr(o, fmt_info, value);
}

bool _write_u64(Str *o, FmtInfo *fmt_info, u64 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    // If this is to be printed as a character sequence
    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 8);
    }

    // Store original length to calculate content size later
    size start_len = o->length;

    // Create temporary buffer for number formatting
    Str temp = StrInit(o->allocator);

    // Determine base based on format flags
    u8 base = 10; // default is decimal
    if (fmt_info->flags & FMT_FLAG_HEX) {
        base = 16;
    } else if (fmt_info->flags & FMT_FLAG_BINARY) {
        base = 2;
    } else if (fmt_info->flags & FMT_FLAG_OCTAL) {
        base = 8;
    }

    // Use StrFromU64 directly with the appropriate base
    bool         use_prefix = (base != 10); // Add prefix for non-decimal bases
    StrIntFormat config = {.base = base, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0, .use_prefix = use_prefix};
    if (!StrFromU64(&temp, *v, &config)) {
        StrDeinit(&temp);
        return false;
    }

    // Merge the formatted number into output
    if (!StrMerge(o, &temp)) {
        StrDeinit(&temp);
        return false;
    }
    StrDeinit(&temp);

    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size content_len = o->length - start_len;
        if (!PadString(o, fmt_info->width, fmt_info->align, content_len)) {
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

    // If this is to be printed as a character sequence
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

    // If this is to be printed as a character sequence
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

    // If this is to be printed as a character sequence
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

    // If this is to be printed as a character sequence
    if (fmt_info->flags & FMT_FLAG_CHAR) {
        return write_int_as_chars(o, fmt_info->flags, *v, 8);
    }

    // Store original length to calculate content size later
    size start_len = o->length;

    // Create temporary buffer for number formatting
    Str temp = StrInit(o->allocator);

    // Determine base based on format flags
    u8 base = 10; // default is decimal
    if (fmt_info->flags & FMT_FLAG_HEX) {
        base = 16;
    } else if (fmt_info->flags & FMT_FLAG_BINARY) {
        base = 2;
    } else if (fmt_info->flags & FMT_FLAG_OCTAL) {
        base = 8;
    }

    // Use StrFromI64 directly with the appropriate base
    bool         use_prefix = (base != 10); // Add prefix for non-decimal bases
    StrIntFormat config = {.base = base, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0, .use_prefix = use_prefix};
    if (!StrFromI64(&temp, *v, &config)) {
        StrDeinit(&temp);
        return false;
    }

    // Merge the formatted number into output
    if (!StrMerge(o, &temp)) {
        StrDeinit(&temp);
        return false;
    }
    StrDeinit(&temp);

    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size content_len = o->length - start_len;
        if (!PadString(o, fmt_info->width, fmt_info->align, content_len)) {
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

    // If this is to be printed as a character sequence
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

    // If this is to be printed as a character sequence
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

    // If this is to be printed as a character sequence
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

    // If this is to be printed as a character sequence
    if (fmt_info->flags & FMT_FLAG_CHAR) {
        // Convert the 64-bit float value to a 64-bit integer for byte access
        u64 bits;
        MemCopy(&bits, v, sizeof(bits)); // Avoid type punning issues
        return write_int_as_chars(o, fmt_info->flags, bits, 8);
    }

    // Store original length to calculate content size later
    size start_len = o->length;

    // Handle special cases directly here to avoid StrFromF64 issues
    if (isnan(*v)) {
        // Direct string append for NaN
        if (fmt_info->flags & FMT_FLAG_CAPS) {
            if (!StrPushBackZstr(o, "NAN")) {
                return false;
            }
        } else {
            if (!StrPushBackZstr(o, "nan")) {
                return false;
            }
        }
    } else if (isinf(*v)) {
        // Direct string append for infinity
        if (*v < 0) {
            if (!StrPushBack(o, '-')) {
                return false;
            }
        }

        if (fmt_info->flags & FMT_FLAG_CAPS) {
            if (!StrPushBackZstr(o, "INF")) {
                return false;
            }
        } else {
            if (!StrPushBackZstr(o, "inf")) {
                return false;
            }
        }
    } else {
        // Normal case - use StrFromF64
        // Create temporary buffer for number formatting
        Str temp = StrInit(o->allocator);

        // Use StrFromF64 directly with the appropriate parameters
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

        // Merge the formatted number into output
        if (!StrMerge(o, &temp)) {
            StrDeinit(&temp);
            return false;
        }
        StrDeinit(&temp);
    }

    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size content_len = o->length - start_len;
        if (!PadString(o, fmt_info->width, fmt_info->align, content_len)) {
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

    // If this is to be printed as a character sequence
    if (fmt_info->flags & FMT_FLAG_CHAR) {
        // Convert the 32-bit float value to a 32-bit integer for byte access
        u32 bits;
        MemCopy(&bits, v, sizeof(bits)); // Avoid type punning issues
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

    if (FloatFmtUsesUnsupportedFlags(fmt_info)) {
        LOG_FATAL("Float only supports decimal and scientific formatting");
    }

    start_len = o->length;
    if (fmt_info->flags & FMT_FLAG_SCIENTIFIC) {
        if (!FloatFmtTryToScientificStr(
                &temp,
                value,
                fmt_info->precision,
                (fmt_info->flags & FMT_FLAG_HAS_PRECISION) != 0,
                (fmt_info->flags & FMT_FLAG_CAPS) != 0,
                o->allocator
            )) {
            return false;
        }
    } else {
        if (!FloatFmtTryToDecimalStr(
                &temp,
                value,
                fmt_info->precision,
                (fmt_info->flags & FMT_FLAG_HAS_PRECISION) != 0,
                o->allocator
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
        size content_len = o->length - start_len;
        if (!PadString(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}
#endif // FEATURE_FLOAT

// Helper function to handle escape sequences
static char ProcessEscape(const char **str) {
    if (!str || !*str)
        return 0;

    const char *s = *str;
    if (*s != '\\') {
        LOG_ERROR("ProcessEscape called on non-escape sequence");
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
            int hex_val = hex_byte(s[0], s[1]);
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

const char *_read_Str(const char *i, FmtInfo *fmt_info, Str *s) {
    if (!i || !s)
        LOG_FATAL("Invalid arguments");

    ValidateStr(s);

    // Check for case conversion flags
    bool force_case = fmt_info && (fmt_info->flags & FMT_FLAG_FORCE_CASE) != 0;
    bool is_caps    = fmt_info && (fmt_info->flags & FMT_FLAG_CAPS) != 0;
    bool is_string  = fmt_info && (fmt_info->flags & FMT_FLAG_STRING) != 0;
    u32  r          = fmt_info->max_read_len;

    // Check for empty input
    if (!*i || !r) {
        LOG_ERROR("Empty input string");
        return i;
    }

    // Check for quoted string
    char quote = 0;
    if (is_string && (*i == '"' || *i == '\'')) {
        quote = *i++;
        r--;
    }

    while (r && *i) {
        if (quote) {
            // Quoted string mode
            if (*i == '\\') {
                const char *curr = i;
                char        c    = ProcessEscape(&curr);
                if (c == 0) { // Error in escape sequence
                    StrDeinit(s);
                    return NULL;
                }
                i  = curr + 1; // Move past the escape sequence
                r -= 2;

                // Apply case conversion if needed
                if (force_case) {
                    c = is_caps ? TO_UPPER(c) : TO_LOWER(c);
                }

                StrPushBack(s, c);
            } else if (*i == quote) {
                i++;      // Skip closing quote
                r--;
                return i; // Successfully read quoted string
            } else {
                char c = *i++;
                r--;

                // Apply case conversion if needed
                if (force_case) {
                    c = is_caps ? TO_UPPER(c) : TO_LOWER(c);
                }

                StrPushBack(s, c);
            }
        } else {
            // Unquoted string mode - read until whitespace
            if (is_string && IS_SPACE(*i)) {
                return i; // Successfully read unquoted string
            }

            if (*i == '\\') {
                const char *curr = i;
                char        c    = ProcessEscape(&curr);
                if (c == 0) { // Error in escape sequence
                    StrDeinit(s);
                    return NULL;
                }
                i  = curr + 1; // Move past the escape sequence
                r -= 2;

                // Apply case conversion if needed
                if (force_case) {
                    c = is_caps ? TO_UPPER(c) : TO_LOWER(c);
                }

                StrPushBack(s, c);
            } else {
                char c = *i++;
                r--;

                // Apply case conversion if needed
                if (force_case) {
                    c = is_caps ? TO_UPPER(c) : TO_LOWER(c);
                }

                StrPushBack(s, c);
            }
        }
    }

    // If we get here with a quote, the string was unterminated
    if (quote) {
        LOG_ERROR("Unterminated quoted string");
        StrDeinit(s);
        return NULL;
    }

    // Successfully read unquoted string that ended at EOF
    return i;
}

// Helper function to check if a character is valid for number parsing
static bool IsValidNumberChar(char c, bool is_first_char, bool allow_decimal) {
    // Allow digits
    if (IS_DIGIT(c))
        return true;

    // Allow signs only at the beginning
    if ((c == '+' || c == '-') && is_first_char)
        return true;

    // Allow decimal point if allowed
    if (c == '.' && allow_decimal)
        return true;

    // Allow base prefixes (only at the beginning or after a sign)
    if (c == '0' && (is_first_char || (is_first_char + 1)))
        return true;

    // Allow 'x', 'b', 'o' for hex/binary/octal after '0'
    // Note: We only get here for the second character in "0x", "0b", "0o"
    if (!is_first_char && (c == 'x' || c == 'X' || c == 'b' || c == 'B' || c == 'o' || c == 'O')) {
        // Check if the previous character was '0'
        // This will be validated by the base 0 parsing logic later
        return true;
    }

    // Allow hex letters only for hexadecimal base (after 0x)
    // This is handled by the base detection in the parsing functions
    // We will just collect characters here and let the parser reject invalid ones
    if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
        return true;
    }

    // Allow 'e' or 'E' for scientific notation if decimal is allowed
    if (allow_decimal && (c == 'e' || c == 'E'))
        return true;

    return false;
}

// Create a helper function to check if the parsed string contains only valid numeric characters
static bool IsValidNumericString(const Str *str, bool allow_float) {
    if (!str || !str->data)
        return false;

    // Empty string is invalid
    if (str->length == 0)
        return false;

    // Special floating point values: inf, nan
    if (allow_float) {
        if (str->length == 3) {
            if ((str->data[0] == 'i' || str->data[0] == 'I') && (str->data[1] == 'n' || str->data[1] == 'N') &&
                (str->data[2] == 'f' || str->data[2] == 'F')) {
                return true; // "inf"
            }
            if ((str->data[0] == 'n' || str->data[0] == 'N') && (str->data[1] == 'a' || str->data[1] == 'A') &&
                (str->data[2] == 'n' || str->data[2] == 'N')) {
                return true; // "nan"
            }
        }

        if (str->length == 4 && str->data[0] == '-') {
            if ((str->data[1] == 'i' || str->data[1] == 'I') && (str->data[2] == 'n' || str->data[2] == 'N') &&
                (str->data[3] == 'f' || str->data[3] == 'F')) {
                return true; // "-inf"
            }
        }
    }

    // Check for decimal, octal, hex, or binary prefix
    bool is_hex = false;
    bool is_bin = false;
    bool is_oct = false;

    if (str->length > 2 && str->data[0] == '0') {
        if (str->data[1] == 'x' || str->data[1] == 'X') {
            is_hex = true;
        } else if (str->data[1] == 'b' || str->data[1] == 'B') {
            is_bin = true;
        } else if (str->data[1] == 'o' || str->data[1] == 'O') {
            is_oct = true;
        }
    }

    // For floating point, we need to track decimal point and scientific notation
    bool has_decimal = false;
    bool has_exp     = false;

    // Check each character
    for (size i = 0; i < str->length; i++) {
        char c = str->data[i];

        // Skip prefix
        if ((is_hex || is_bin || is_oct) && (i == 0 || i == 1)) {
            continue;
        }

        // Check sign (only valid at start or after 'e'/'E')
        if ((c == '+' || c == '-') &&
            (i == 0 || (allow_float && has_exp && (i > 0 && (str->data[i - 1] == 'e' || str->data[i - 1] == 'E'))))) {
            continue;
        }

        // Check decimal point (only for float and only once)
        if (allow_float && c == '.') {
            if (has_decimal) {
                return false; // Second decimal point
            }
            has_decimal = true;
            continue;
        }

        // Check scientific notation (only for float and only once)
        if (allow_float && (c == 'e' || c == 'E')) {
            if (has_exp) {
                return false; // Second exponent
            }
            has_exp = true;
            continue;
        }

        // Check digits
        if (c >= '0' && c <= '9') {
            continue;
        }

        // Check hex digits
        if (is_hex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            continue;
        }

        // Check binary digits
        if (is_bin && (c == '0' || c == '1')) {
            continue;
        }

        // Check octal digits
        if (is_oct && (c >= '0' && c <= '7')) {
            continue;
        }

        // Any other character is invalid
        return false;
    }

    // Validate special requirements for float
    if (allow_float) {
        // If we have an exponent, make sure it's not at the end
        if (has_exp) {
            char last_char = str->data[str->length - 1];
            if (last_char == 'e' || last_char == 'E' || last_char == '+' || last_char == '-') {
                return false; // Incomplete exponent
            }
        }
    }

    return true;
}

const char *_read_f64(const char *i, FmtInfo *fmt_info, f64 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        u64         temp = 0;
        const char *next = read_chars_internal(i, (u8 *)&temp, sizeof(temp), fmt_info);
        *v               = (f64)temp;
        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse f64: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }

    // Check for special values (inf, nan)
    if ((*i == 'i' || *i == 'I' || *i == 'n' || *i == 'N') || (*i == '-' && (*(i + 1) == 'i' || *(i + 1) == 'I'))) {
        // For special values, use the original approach
        const char *start = i;
        while (*i && !IS_SPACE(*i))
            i++;

        // Create a temporary Str for parsing
        Str temp = StrInitFromCstr(start, i - start, &scratch);

        // Try to parse as special value
        if (StrToF64(&temp, v, NULL)) {
            StrDeinit(&temp);
            DefaultAllocatorDeinit(&scratch);
            return i;
        }
        StrDeinit(&temp);

        // If parsing failed, fall back to the new approach
        i = start;
    }

    // Find the end of the number using more precise rules
    const char *start       = i;
    size        pos         = 0;
    bool        has_decimal = false; // Track if we've seen a decimal point

    // Parse character by character
    while (i[pos]) {
        // Only allow one decimal point
        if (i[pos] == '.') {
            if (has_decimal)
                break; // Second decimal point - stop
            has_decimal = true;
        }

        // If we see an 'e' or 'E', check if it's followed by a valid exponent
        if ((i[pos] == 'e' || i[pos] == 'E') && pos > 0) {
            // Move to the next character after 'e'
            pos++;

            // Allow sign in exponent
            if (i[pos] == '+' || i[pos] == '-')
                pos++;

            // Must have at least one digit in exponent
            if (!IS_DIGIT(i[pos])) {
                // Invalid exponent - back up to before the 'e'
                pos--;
                break;
            }

            // Continue with exponent digits
            while (IS_DIGIT(i[pos]))
                pos++;
            break; // Stop after exponent
        }

        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, true)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Validate the string is a proper floating point number
    if (!IsValidNumericString(&temp, true)) {
        LOG_ERROR("Invalid floating point format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use StrToF64 directly
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

const char *_read_u8(const char *i, FmtInfo *fmt_info, u8 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse u8: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        const char *next = read_chars_internal(i, (u8 *)v, sizeof(*v), fmt_info);
        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Find the end of the number using more precise rules
    const char *start = i;
    size        pos   = 0;

    // Parse character by character
    while (i[pos]) {
        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, false)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Check for special prefixes with no digits
    if (temp.length == 2 && temp.data[0] == '0' &&
        (temp.data[1] == 'x' || temp.data[1] == 'X' || temp.data[1] == 'b' || temp.data[1] == 'B' ||
         temp.data[1] == 'o' || temp.data[1] == 'O')) {
        LOG_ERROR("Incomplete number format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Validate the string is a proper number
    if (!IsValidNumericString(&temp, false)) {
        LOG_ERROR("Invalid numeric format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use base 0 to let strtoul detect the base from prefix
    u64 val;
    if (!StrToU64(&temp, &val, NULL)) {
        LOG_ERROR("Failed to parse u8");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Check for overflow
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

const char *_read_u16(const char *i, FmtInfo *fmt_info, u16 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        *v               = 0;
        const char *next = read_chars_internal(i, (u8 *)v, sizeof(*v), fmt_info);

        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse u16: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }

    // Find the end of the number using more precise rules
    const char *start = i;
    size        pos   = 0;

    // Parse character by character
    while (i[pos]) {
        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, false)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Check for special prefixes with no digits
    if (temp.length == 2 && temp.data[0] == '0' &&
        (temp.data[1] == 'x' || temp.data[1] == 'X' || temp.data[1] == 'b' || temp.data[1] == 'B' ||
         temp.data[1] == 'o' || temp.data[1] == 'O')) {
        LOG_ERROR("Incomplete number format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Validate the string is a proper number
    if (!IsValidNumericString(&temp, false)) {
        LOG_ERROR("Invalid numeric format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use base 0 to let strtoul detect the base from prefix
    u64 val;
    if (!StrToU64(&temp, &val, NULL)) {
        LOG_ERROR("Failed to parse u16");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Check for overflow
    if (val > UINT16_MAX) {
        LOG_ERROR("Value {} exceeds u16 maximum ({})", val, UINT16_MAX);
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    *v = (u16)val;
    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

const char *_read_u32(const char *i, FmtInfo *fmt_info, u32 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        *v               = 0;
        const char *next = read_chars_internal(i, (u8 *)v, sizeof(*v), fmt_info);

        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse u32: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }
    // Find the end of the number using more precise rules
    const char *start = i;
    size        pos   = 0;

    // Parse character by character
    while (i[pos]) {
        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, false)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Check for special prefixes with no digits
    if (temp.length == 2 && temp.data[0] == '0' &&
        (temp.data[1] == 'x' || temp.data[1] == 'X' || temp.data[1] == 'b' || temp.data[1] == 'B' ||
         temp.data[1] == 'o' || temp.data[1] == 'O')) {
        LOG_ERROR("Incomplete number format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Validate the string is a proper number
    if (!IsValidNumericString(&temp, false)) {
        LOG_ERROR("Invalid numeric format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use base 0 to let strtoul detect the base from prefix
    u64 val;
    if (!StrToU64(&temp, &val, NULL)) {
        LOG_ERROR("Failed to parse u32");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Check for overflow
    if (val > UINT32_MAX) {
        LOG_ERROR("Value {} exceeds u32 maximum ({})", val, UINT32_MAX);
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    *v = (u32)val;
    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

const char *_read_u64(const char *i, FmtInfo *fmt_info, u64 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        *v               = 0;
        const char *next = read_chars_internal(i, (u8 *)v, sizeof(*v), fmt_info);

        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse u64: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }

    // Find the end of the number using more precise rules
    const char *start = i;
    size        pos   = 0;

    // Parse character by character
    while (i[pos]) {
        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, false)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Check for special prefixes with no digits
    if (temp.length == 2 && temp.data[0] == '0' &&
        (temp.data[1] == 'x' || temp.data[1] == 'X' || temp.data[1] == 'b' || temp.data[1] == 'B' ||
         temp.data[1] == 'o' || temp.data[1] == 'O')) {
        LOG_ERROR("Incomplete number format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Validate the string is a proper number
    if (!IsValidNumericString(&temp, false)) {
        LOG_ERROR("Invalid numeric format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use base 0 to let strtoul detect the base from prefix
    if (!StrToU64(&temp, v, NULL)) {
        LOG_ERROR("Failed to parse u64");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

const char *_read_i8(const char *i, FmtInfo *fmt_info, i8 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        *v               = 0;
        const char *next = read_chars_internal(i, (u8 *)v, sizeof(*v), fmt_info);
        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse i8: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }

    // Find the end of the number using more precise rules
    const char *start = i;
    size        pos   = 0;

    // Parse character by character
    while (i[pos]) {
        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, false)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Check for special prefixes with no digits
    if (temp.length == 2 && temp.data[0] == '0' &&
        (temp.data[1] == 'x' || temp.data[1] == 'X' || temp.data[1] == 'b' || temp.data[1] == 'B' ||
         temp.data[1] == 'o' || temp.data[1] == 'O')) {
        LOG_ERROR("Incomplete number format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Validate the string is a proper number
    if (!IsValidNumericString(&temp, false)) {
        LOG_ERROR("Invalid numeric format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use base 0 to let strtoul detect the base from prefix
    i64 val;
    if (!StrToI64(&temp, &val, NULL)) {
        LOG_ERROR("Failed to parse i8");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Check for overflow/underflow
    if (val > INT8_MAX || val < INT8_MIN) {
        LOG_ERROR("Value {} outside i8 range ({} to {})", val, INT8_MIN, INT8_MAX);
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    *v = (i8)val;
    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

const char *_read_i16(const char *i, FmtInfo *fmt_info, i16 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        *v               = 0;
        const char *next = read_chars_internal(i, (u8 *)v, sizeof(*v), fmt_info);

        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse i16: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }

    // Find the end of the number using more precise rules
    const char *start = i;
    size        pos   = 0;

    // Parse character by character
    while (i[pos]) {
        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, false)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Check for special prefixes with no digits
    if (temp.length == 2 && temp.data[0] == '0' &&
        (temp.data[1] == 'x' || temp.data[1] == 'X' || temp.data[1] == 'b' || temp.data[1] == 'B' ||
         temp.data[1] == 'o' || temp.data[1] == 'O')) {
        LOG_ERROR("Incomplete number format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Validate the string is a proper number
    if (!IsValidNumericString(&temp, false)) {
        LOG_ERROR("Invalid numeric format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use base 0 to let strtoul detect the base from prefix
    i64 val;
    if (!StrToI64(&temp, &val, NULL)) {
        LOG_ERROR("Failed to parse i16");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Check for overflow/underflow
    if (val > INT16_MAX || val < INT16_MIN) {
        LOG_ERROR("Value {} outside i16 range ({} to {})", val, INT16_MIN, INT16_MAX);
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    *v = (i16)val;
    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

const char *_read_i32(const char *i, FmtInfo *fmt_info, i32 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        *v               = 0;
        const char *next = read_chars_internal(i, (u8 *)v, sizeof(*v), fmt_info);

        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse i32: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }

    // Find the end of the number using more precise rules
    const char *start = i;
    size        pos   = 0;

    // Parse character by character
    while (i[pos]) {
        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, false)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Check for special prefixes with no digits
    if (temp.length == 2 && temp.data[0] == '0' &&
        (temp.data[1] == 'x' || temp.data[1] == 'X' || temp.data[1] == 'b' || temp.data[1] == 'B' ||
         temp.data[1] == 'o' || temp.data[1] == 'O')) {
        LOG_ERROR("Incomplete number format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Validate the string is a proper number
    if (!IsValidNumericString(&temp, false)) {
        LOG_ERROR("Invalid numeric format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use base 0 to let strtoul detect the base from prefix
    i64 val;
    if (!StrToI64(&temp, &val, NULL)) {
        LOG_ERROR("Failed to parse i32");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Check for overflow/underflow
    if (val > INT32_MAX || val < INT32_MIN) {
        LOG_ERROR("Value {} outside i32 range ({} to {})", val, INT32_MIN, INT32_MAX);
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    *v = (i32)val;
    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

const char *_read_i64(const char *i, FmtInfo *fmt_info, i64 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        *v               = 0;
        const char *next = read_chars_internal(i, (u8 *)v, sizeof(*v), fmt_info);

        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse i64: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }

    // Find the end of the number using more precise rules
    const char *start = i;
    size        pos   = 0;

    // Parse character by character
    while (i[pos]) {
        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, false)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Check for special prefixes with no digits
    if (temp.length == 2 && temp.data[0] == '0' &&
        (temp.data[1] == 'x' || temp.data[1] == 'X' || temp.data[1] == 'b' || temp.data[1] == 'B' ||
         temp.data[1] == 'o' || temp.data[1] == 'O')) {
        LOG_ERROR("Incomplete number format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Validate the string is a proper number
    if (!IsValidNumericString(&temp, false)) {
        LOG_ERROR("Invalid numeric format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use base 0 to let strtoul detect the base from prefix
    if (!StrToI64(&temp, v, NULL)) {
        LOG_ERROR("Failed to parse i64");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    StrDeinit(&temp);
    DefaultAllocatorDeinit(&scratch);
    return start + pos;
}

const char *_read_Zstr(const char *i, FmtInfo *fmt_info, const char **out) {
    (void)fmt_info;
    (void)out;
    LOG_FATAL("_read_Zstr requires explicit allocator provenance; use _read_ZstrAlloc / ZstrIO(zstr, alloc) instead.");
    return i;
}

const char *_read_ZstrAlloc(const char *i, FmtInfo *fmt_info, ZstrIOArg *arg) {
    char      **out           = NULL;
    char       *previous      = NULL;
    char       *result        = NULL;
    const char *next          = NULL;
    Allocator  *allocator_ptr = NULL;
    Str         temp;
    FmtInfo     default_fmt;

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

    result = ZstrDupN(temp.data, temp.length, allocator_ptr);
    if (!result) {
        LOG_ERROR("Failed to allocate memory for string");
        StrDeinit(&temp);
        return i;
    }

    if (previous) {
        AllocatorFree(allocator_ptr, previous, ZstrLen(previous) + 1);
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

    // Store original length to calculate content size later
    size start_len = o->length;

    if (fmt_info->flags & FMT_FLAG_HEX) {
        // Format as hexadecimal
        if (bv->length == 0) {
            if (!StrPushBackZstr(o, "0x0")) {
                return false;
            }
        } else {
            // Convert to integer (up to 64 bits) and format as hex
            u64          value  = BitVecToInteger(bv);
            StrIntFormat config = {.base = 16, .uppercase = (fmt_info->flags & FMT_FLAG_CAPS) != 0, .use_prefix = true};
            if (!StrFromU64(o, value, &config)) {
                return false;
            }
        }
    } else if (fmt_info->flags & FMT_FLAG_OCTAL) {
        // Format as octal
        if (bv->length == 0) {
            if (!StrPushBackZstr(o, "0o0")) {
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
        // Default: Format as binary string (e.g., "10110")
        if (bv->length == 0) {
            // Empty BitVec - don't output anything
        } else {
            Str bit_str;

            if (!bitvec_try_to_str(&bit_str, bv, o->allocator)) {
                return false;
            }
            if (!StrMerge(o, &bit_str)) {
                StrDeinit(&bit_str);
                return false;
            }
            StrDeinit(&bit_str);
        }
    }

    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size content_len = o->length - start_len;
        if (!PadString(o, fmt_info->width, fmt_info->align, content_len)) {
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

        u8 *buffer = (u8 *)AllocatorAlloc(o->allocator, byte_len * sizeof(u8), true);

        if (!buffer) {
            LOG_ERROR("Failed to allocate buffer for Int character formatting");
            return false;
        }

        (void)IntToBytesBE(value, buffer, byte_len);
        if (!write_char_internal(o, fmt_info->flags, (const char *)buffer, byte_len)) {
            AllocatorFree(o->allocator, buffer, byte_len * sizeof(u8));
            return false;
        }
        AllocatorFree(o->allocator, buffer, byte_len * sizeof(u8));
        return true;
    }

    size start_len = o->length;
    Str  temp;
    u8   radix = IntFmtRadixFromFlags(fmt_info);

    if (radix == 10) {
        if (!int_try_to_str(&temp, value, o->allocator)) {
            return false;
        }
    } else {
        if (!int_try_to_str_radix(&temp, value, radix, (fmt_info->flags & FMT_FLAG_CAPS) != 0, o->allocator)) {
            return false;
        }
    }

    if (!StrMerge(o, &temp)) {
        StrDeinit(&temp);
        return false;
    }
    StrDeinit(&temp);

    if (fmt_info->width > 0) {
        size content_len = o->length - start_len;
        if (!PadString(o, fmt_info->width, fmt_info->align, content_len)) {
            return false;
        }
    }

    return true;
}
#endif // FEATURE_INT

bool _write_UnsupportedType(Str *o, FmtInfo *fmt_info, const char **s) {
    (void)o;
    (void)fmt_info;
    (void)s;
    LOG_FATAL("Attempt to write unsupported type");
    return false;
}

#if FEATURE_BITVEC
const char *_read_BitVec(const char *i, FmtInfo *fmt_info, BitVec *bv) {
    (void)fmt_info; // Unused parameter
    if (!i || !bv) {
        LOG_FATAL("Invalid arguments");
        return i;
    }

    ValidateBitVec(bv);

    // Skip leading whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty input
    if (!*i) {
        LOG_ERROR("Empty input string");
        return i;
    }

    const char *start = i;

    // Check for hex format (0x...)
    if (i[0] == '0' && (i[1] == 'x' || i[1] == 'X')) {
        // Read hex value
        i                     += 2; // Skip "0x"
        const char *hex_start  = i;

        // Read hex digits
        while (IS_XDIGIT(*i)) {
            i++;
        }

        if (i == hex_start) {
            LOG_ERROR("Invalid hex format - no digits after 0x");
            return start;
        }

        // Parse hex string
        Str            hex_str = StrInitFromCstr(hex_start, i - hex_start, bv->allocator);
        u64            value;
        StrParseConfig config = {.base = 16};
        if (!StrToU64(&hex_str, &value, &config)) {
            LOG_ERROR("Failed to parse hex value");
            StrDeinit(&hex_str);
            return start;
        }

        // Determine bit length (minimum to represent the value)
        u64 bit_len = value == 0 ? 1 : 64 - count_leading_zeros_u64(value);
        if (bit_len < 4)
            bit_len = 4; // Minimum 4 bits for hex display

        *bv = BitVecFromInteger(value, bit_len, bv->allocator);
        StrDeinit(&hex_str);
        return i;
    }

    // Check for octal format (0o...)
    if (i[0] == '0' && (i[1] == 'o' || i[1] == 'O')) {
        // Read octal value
        i                     += 2; // Skip "0o"
        const char *oct_start  = i;

        // Read octal digits
        while (*i >= '0' && *i <= '7') {
            i++;
        }

        if (i == oct_start) {
            LOG_ERROR("Invalid octal format - no digits after 0o");
            return start;
        }

        // Parse octal string
        Str            oct_str = StrInitFromCstr(oct_start, i - oct_start, bv->allocator);
        u64            value;
        StrParseConfig config = {.base = 8};
        if (!StrToU64(&oct_str, &value, &config)) {
            LOG_ERROR("Failed to parse octal value");
            StrDeinit(&oct_str);
            return start;
        }

        // Determine bit length
        u64 bit_len = value == 0 ? 1 : 64 - count_leading_zeros_u64(value);
        if (bit_len < 3)
            bit_len = 3; // Minimum 3 bits for octal display

        *bv = BitVecFromInteger(value, bit_len, bv->allocator);
        StrDeinit(&oct_str);
        return i;
    }

    // Default: Read as binary string (e.g., "10110")
    const char *bin_start = i;

    // Read binary digits
    while (*i == '0' || *i == '1') {
        i++;
    }

    if (i == bin_start) {
        LOG_ERROR("Invalid binary format - expected 0s and 1s");
        return start;
    }

    // Create string from binary digits (already null-terminated by StrInitFromCstr)
    Str bin_str = StrInitFromCstr(bin_start, i - bin_start, bv->allocator);

    // Convert to BitVec using the null-terminated string
    *bv = BitVecFromStr(bin_str.data, bv->allocator);

    StrDeinit(&bin_str);
    return i;
}
#endif // FEATURE_BITVEC

#if FEATURE_INT
const char *_read_Int(const char *i, FmtInfo *fmt_info, Int *value) {
    if (!i || !value) {
        LOG_FATAL("Invalid arguments");
    }

    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        LOG_ERROR("Character-format reads are not supported for Int");
        return i;
    }

    ValidateInt(value);

    while (IS_SPACE(*i)) {
        i++;
    }

    if (!*i) {
        LOG_ERROR("Failed to parse Int: empty input");
        return i;
    }

    const char *start        = i;
    const char *digits_start = i;
    u8          radix        = IntFmtRadixFromFlags(fmt_info);

    if (*digits_start == '+') {
        digits_start++;
        i++;
    }

    if (radix == 16 && digits_start[0] == '0' && (digits_start[1] == 'x' || digits_start[1] == 'X')) {
        LOG_ERROR("Int hex reads expect plain hex digits without a 0x prefix");
        return start;
    }
    if (radix == 2 && digits_start[0] == '0' && (digits_start[1] == 'b' || digits_start[1] == 'B')) {
        LOG_ERROR("Int binary reads expect plain binary digits without a 0b prefix");
        return start;
    }
    if (radix == 8 && digits_start[0] == '0' && (digits_start[1] == 'o' || digits_start[1] == 'O')) {
        LOG_ERROR("Int octal reads expect plain octal digits without a 0o prefix");
        return start;
    }

    while (*i && IntFmtDigitMatchesRadix(*i, radix)) {
        i++;
    }

    if (i == digits_start) {
        LOG_ERROR("Failed to parse Int");
        return start;
    }

    if (*i == '_') {
        LOG_ERROR("Int reads do not accept digit separators");
        return start;
    }

    Str  temp   = StrInitFromCstr(start, i - start, value->bits.allocator);
    Int  parsed = IntInit(value->bits.allocator);
    bool ok     = IntTryFromStrRadix(&parsed, temp.data, radix);

    if (!ok) {
        StrDeinit(&temp);
        return start;
    }

    IntDeinit(value);
    *value = parsed;

    StrDeinit(&temp);
    return i;
}
#endif // FEATURE_INT

#if FEATURE_FLOAT
const char *_read_Float(const char *i, FmtInfo *fmt_info, Float *value) {
    size        token_len = 0;
    const char *start     = NULL;
    Str         temp;
    Float       parsed;

    if (!i || !value) {
        LOG_FATAL("Invalid arguments");
    }

    temp   = StrInit(value->significand.bits.allocator);
    parsed = FloatInit(value->significand.bits.allocator);

    if (FloatFmtUsesUnsupportedFlags(fmt_info)) {
        LOG_ERROR("Float only supports decimal and scientific reading");
        StrDeinit(&temp);
        FloatDeinit(&parsed);
        return i;
    }

    ValidateFloat(value);

    while (IS_SPACE(*i)) {
        i++;
    }

    if (!*i) {
        LOG_ERROR("Failed to parse Float: empty input");
        StrDeinit(&temp);
        FloatDeinit(&parsed);
        return i;
    }

    start     = i;
    token_len = FloatFmtTokenLength(start);

    if (token_len == 0) {
        LOG_ERROR("Failed to parse Float");
        StrDeinit(&temp);
        FloatDeinit(&parsed);
        return start;
    }

    StrDeinit(&temp);
    temp = StrInitFromCstr(start, token_len, value->significand.bits.allocator);
    if (!FloatTryFromStr(&parsed, temp.data)) {
        StrDeinit(&temp);
        FloatDeinit(&parsed);
        return start;
    }

    FloatDeinit(value);
    *value = parsed;

    StrDeinit(&temp);
    return start + token_len;
}
#endif              // FEATURE_FLOAT

const char *_read_UnsupportedType(const char *i, FmtInfo *fmt_info, const char **s) {
    (void)fmt_info; // Unused parameter
    (void)s;
    LOG_FATAL("Attempt to read unsupported type.");
    return i;
}

const char *_read_f32(const char *i, FmtInfo *fmt_info, f32 *v) {
    DefaultAllocator scratch = DefaultAllocatorInit();

    if (!i || !v) {
        DefaultAllocatorDeinit(&scratch);
        LOG_FATAL("Invalid arguments");
    }

    // Handle character format specifier
    if (fmt_info && (fmt_info->flags & FMT_FLAG_CHAR)) {
        u32         temp = 0;
        const char *next = read_chars_internal(i, (u8 *)&temp, sizeof(temp), fmt_info);
        *v               = (f32)temp;
        DefaultAllocatorDeinit(&scratch);
        return next;
    }

    // Skip whitespace
    while (IS_SPACE(*i))
        i++;

    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse f32: empty input");
        DefaultAllocatorDeinit(&scratch);
        return i;
    }

    // Check for special values (inf, nan)
    if ((*i == 'i' || *i == 'I' || *i == 'n' || *i == 'N') || (*i == '-' && (*(i + 1) == 'i' || *(i + 1) == 'I'))) {
        // For special values, use the original approach
        const char *start = i;
        while (*i && !IS_SPACE(*i))
            i++;

        // Create a temporary Str for parsing
        Str temp = StrInitFromCstr(start, i - start, &scratch);

        // Try to parse as special value
        f64 val;
        if (StrToF64(&temp, &val, NULL)) {
            *v = (f32)val;
            StrDeinit(&temp);
            DefaultAllocatorDeinit(&scratch);
            return i;
        }
        StrDeinit(&temp);

        // If parsing failed, fall back to the new approach
        i = start;
    }

    // Find the end of the number using more precise rules
    const char *start       = i;
    size        pos         = 0;
    bool        has_decimal = false; // Track if we've seen a decimal point

    // Parse character by character
    while (i[pos]) {
        // Only allow one decimal point
        if (i[pos] == '.') {
            if (has_decimal)
                break; // Second decimal point - stop
            has_decimal = true;
        }

        // If we see an 'e' or 'E', check if it's followed by a valid exponent
        if ((i[pos] == 'e' || i[pos] == 'E') && pos > 0) {
            // Move to the next character after 'e'
            pos++;

            // Allow sign in exponent
            if (i[pos] == '+' || i[pos] == '-')
                pos++;

            // Must have at least one digit in exponent
            if (!IS_DIGIT(i[pos])) {
                // Invalid exponent - back up to before the 'e'
                pos--;
                break;
            }

            // Continue with exponent digits
            while (IS_DIGIT(i[pos]))
                pos++;
            break; // Stop after exponent
        }

        // Check if character is valid for a number
        if (!IsValidNumberChar(i[pos], pos == 0, true)) {
            break;
        }

        pos++;
    }

    // Create a temporary Str for parsing
    Str temp = StrInitFromCstr(start, pos, &scratch);

    // Validate the string is a proper floating point number
    if (!IsValidNumericString(&temp, true)) {
        LOG_ERROR("Invalid floating point format");
        StrDeinit(&temp);
        DefaultAllocatorDeinit(&scratch);
        return start;
    }

    // Use StrToF64 directly
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

    return StrPushBack(o, *v);
}

static bool _write_r16(Str *o, FmtInfo *fmt_info, u16 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    // if native endianness provided, then deduce endianness and set correspondingly
    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }

    u16 x = *v;
    switch (fmt_info->endian) {
        case ENDIAN_BIG : {
            return StrPushBack(o, ((x >> 8) & 0xff)) && StrPushBack(o, (x & 0xff));
        }
        case ENDIAN_LITTLE : {
            return StrPushBack(o, (x & 0xff)) && StrPushBack(o, ((x >> 8) & 0xff));
        }
        case ENDIAN_NATIVE :
        default : {
            LOG_FATAL("Invalid endianness provided. Unexpected code reached.");
            return false;
        }
    }
}

static bool _write_r32(Str *o, FmtInfo *fmt_info, u32 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    // if native endianness provided, then deduce endianness and set correspondingly
    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }


    u32 x = *v;
    switch (fmt_info->endian) {
        case ENDIAN_BIG : {
            return StrPushBack(o, ((x >> 24) & 0xff)) && StrPushBack(o, (x >> 16) & 0xff) &&
                   StrPushBack(o, (x >> 8) & 0xff) && StrPushBack(o, (x & 0xff));
        }
        case ENDIAN_LITTLE : {
            return StrPushBack(o, (x & 0xff)) && StrPushBack(o, (x >> 8) & 0xff) && StrPushBack(o, (x >> 16) & 0xff) &&
                   StrPushBack(o, ((x >> 24) & 0xff));
        }
        case ENDIAN_NATIVE :
        default : {
            LOG_FATAL("Invalid endianness provided. Unexpected code reached.");
            return false;
        }
    }
}


static bool _write_r64(Str *o, FmtInfo *fmt_info, u64 *v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    // if native endianness provided, then deduce endianness and set correspondingly
    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }

    u64 x = *v;
    switch (fmt_info->endian) {
        case ENDIAN_BIG : {
            return StrPushBack(o, ((x >> 56) & 0xff)) && StrPushBack(o, (x >> 48) & 0xff) &&
                   StrPushBack(o, (x >> 40) & 0xff) && StrPushBack(o, (x >> 32) & 0xff) &&
                   StrPushBack(o, (x >> 24) & 0xff) && StrPushBack(o, (x >> 16) & 0xff) &&
                   StrPushBack(o, (x >> 8) & 0xff) && StrPushBack(o, (x & 0xff));
        }
        case ENDIAN_LITTLE : {
            return StrPushBack(o, (x & 0xff)) && StrPushBack(o, (x >> 8) & 0xff) && StrPushBack(o, (x >> 16) & 0xff) &&
                   StrPushBack(o, (x >> 24) & 0xff) && StrPushBack(o, (x >> 32) & 0xff) &&
                   StrPushBack(o, (x >> 40) & 0xff) && StrPushBack(o, (x >> 48) & 0xff) &&
                   StrPushBack(o, ((x >> 56) & 0xff));
        }
        case ENDIAN_NATIVE :
        default : {
            LOG_FATAL("Invalid endianness provided. Unexpected code reached.");
            return false;
        }
    }
}

static const char *_read_r8(const char *i, FmtInfo *fmt_info, u8 *v) {
    if (!i || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
    }

    // Read the byte and assign it to the u8 pointer
    *v = (u8)*i;

    // Return the updated pointer to the input stream (advanced by 1 byte)
    return i + 1;
}

static const char *_read_r16(const char *i, FmtInfo *fmt_info, u16 *v) {
    if (!i || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments to _read_r16");
    }

    // Resolve native endianness if specified.
    if (fmt_info->endian == ENDIAN_NATIVE) {
        fmt_info->endian = IS_LITTLE_ENDIAN() ? ENDIAN_LITTLE : ENDIAN_BIG;
    }

    // Cast the input pointer to unsigned char to avoid sign extension issues.
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
            LOG_FATAL("Invalid endianness provided. Unexpected code reached in _read_r16.");
    }

    *v = TO_NATIVE_ENDIAN2(*v);

    return i + 2; // Advance the stream pointer by 2 bytes.
}

static const char *_read_r32(const char *i, FmtInfo *fmt_info, u32 *v) {
    if (!i || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments to _read_r32");
    }

    // Resolve native endianness if specified.
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
            LOG_FATAL("Invalid endianness provided. Unexpected code reached in _read_r32.");
    }

    *v = TO_NATIVE_ENDIAN4(*v);

    return i + 4; // Advance the stream pointer by 4 bytes.
}

static const char *_read_r64(const char *i, FmtInfo *fmt_info, u64 *v) {
    if (!i || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments to _read_r64");
    }

    // Resolve native endianness if specified.
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
            LOG_FATAL("Invalid endianness provided. Unexpected code reached in _read_r64.");
    }

    *v = TO_NATIVE_ENDIAN8(*v);

    return i + 8; // Advance the stream pointer by 8 bytes.
}
