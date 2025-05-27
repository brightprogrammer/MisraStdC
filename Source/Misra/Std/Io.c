/// file      : std/io.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, All rights reserved.
///
/// formatted reading/writing and other magical stuff

#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>

// stdc
#include <ctype.h>
#include <math.h>

// Helper function to parse format specifiers
static void ParseFormatSpec(const char* spec, size_t len, FmtInfo* fi) {
    // Reset format info
    SetMemory(fi, 0, sizeof(FmtInfo));
    
    // Early return if empty spec
    if (len == 0) return;
    
    const char* p = spec;
    const char* end = spec + len;
    
    // Check for named/positional parameter
    if (isalpha(*p) || isdigit(*p)) {
        while (p < end && (isalnum(*p) || *p == '_')) p++;
    }
    
    // Skip colon for format spec
    if (p < end && *p == ':') {
        p++;
        
        // Handle alignment
        if (p < end) {
            switch (*p) {
                case '<': // Left align
                    fi->align = ALIGN_LEFT;
                    p++;
                    break;
                case '>': // Right align
                    fi->align = ALIGN_RIGHT;
                    p++;
                    break;
                case '^': // Center align
                    fi->align = ALIGN_CENTER;
                    p++;
                    break;
            }
        }
        
        // Handle width
        if (p < end && isdigit(*p)) {
            fi->width = 0;
            while (p < end && isdigit(*p)) {
                fi->width = fi->width * 10 + (*p - '0');
                p++;
            }
        }
        
        // Handle precision
        if (p < end && *p == '.') {
            p++;
            fi->precision = 0;
            while (p < end && isdigit(*p)) {
                fi->precision = fi->precision * 10 + (*p - '0');
                p++;
            }
            fi->has_precision = true;
        }
        
        // Handle format type
        if (p < end) {
            switch (*p) {
                case 'x':
                    fi->is_hex = true;
                    fi->is_caps = false;
                    break;
                case 'X':
                    fi->is_hex = true;
                    fi->is_caps = true;
                    break;
                case 'b':
                    fi->is_binary = true;
                    break;
                case 'o':
                    fi->is_octal = true;
                    break;
                case '?':
                    fi->is_debug = true;
                    break;
                case 'e':
                    fi->is_scientific = true;
                    fi->is_caps = false;
                    break;
                case 'E':
                    fi->is_scientific = true;
                    fi->is_caps = true;
                    break;
            }
        }
    }
}

void StrWriteFmtInternal(Str* o, const char* fmtstr, TypeSpecificIO* argv, size argc) {
    if (!o || !fmtstr) {
        LOG_FATAL("Invalid arguments");
    }

    const char* p = fmtstr;
    size remaining = strlen(fmtstr);
    size arg_index = 0;  // Current argument index for positional params

    while (remaining > 0) {
        if (remaining >= 2 && p[0] == '{' && p[1] == '{') {
            StrPushBack(o, '{');
            p += 2;
            remaining -= 2;
        } else if (remaining >= 2 && p[0] == '}' && p[1] == '}') {
            StrPushBack(o, '}');
            p += 2;
            remaining -= 2;
        } else if (p[0] == '{') {
            p++;
            remaining--;

            const char* start = p;
            size spec_len = 0;

            // Find closing brace
            while (remaining > 0 && *p != '}') {
                p++;
                remaining--;
                spec_len++;
            }

            if (remaining == 0 || *p != '}') {
                LOG_FATAL("Unmatched '{' in format string");
            }

            // Parse the specifier
            char spec_buf[32] = {0};
            if (spec_len >= sizeof(spec_buf)) {
                LOG_FATAL("Format specifier too long");
            }

            if (!argc) {
                LOG_FATAL("More placeholders than arguments");
            }

            memcpy(spec_buf, start, spec_len);
            spec_buf[spec_len] = '\0';

            // Parse format info
            FmtInfo fi = {0};
            ParseFormatSpec(spec_buf, spec_len, &fi);

            // Get the argument to format
            size target_arg = arg_index;
            
            // Check for positional parameter
            if (isdigit(spec_buf[0]) || (spec_buf[0] == '-' && isdigit(spec_buf[1]))) {
                char* endptr;
                long pos = strtol(spec_buf, &endptr, 10);
                if (pos < 0 || endptr == spec_buf) {
                    LOG_FATAL("Invalid position index");
                }
                target_arg = (size)pos;
            }
            // Named parameters would be handled here
            
            if (target_arg >= argc) {
                LOG_FATAL("Positional parameter index out of range");
            }
            
            // Write the argument
            TypeSpecificIO* io = &argv[target_arg];
            if (!io->writer) {
                LOG_FATAL("Missing writer function");
            }
            io->writer(o, &fi, io->data);
            
            // Advance argument index for next non-positional parameter
            if (!isdigit(spec_buf[0]) && spec_buf[0] != '-') {
                arg_index++;
            }

            // Consume closing '}'
            p++;
            remaining--;
        } else {
            StrPushBack(o, *p);
            p++;
            remaining--;
        }
    }
}

const char *StrReadFmtInternal(const char *input, const char *fmtstr, TypeSpecificIO *argv, size argc) {
    if (!input || !fmtstr) {
        LOG_FATAL("Invalid arguments");
    }

    const char *p         = fmtstr;
    size        remaining = strlen(fmtstr);
    const char *in        = input;

    while (remaining > 0) {
        if (remaining >= 2 && p[0] == '{' && p[1] == '{') {
            if (!in || *in != '{') {
                LOG_FATAL("Expected '{' in input");
            }
            in++;
            p         += 2;
            remaining -= 2;
        } else if (remaining >= 2 && p[0] == '}' && p[1] == '}') {
            if (!in || *in != '}') {
                LOG_FATAL("Expected '}' in input");
            }
            in++;
            p         += 2;
            remaining -= 2;
        } else if (p[0] == '{') {
            p++;
            remaining--;

            const char *start    = p;
            size        spec_len = 0;

            while (remaining > 0 && *p != '}') {
                p++;
                remaining--;
                spec_len++;
            }

            if (remaining == 0 || *p != '}') {
                LOG_FATAL("Unmatched '{' in format string");
            }

            // Parse optional specifier (ignored for now)
            char spec_buf[32] = {0};
            if (spec_len >= sizeof(spec_buf)) {
                LOG_FATAL("Format specifier too long");
            }

            if (!argc) {
                LOG_FATAL("More placeholders than arguments");
            }

            memcpy(spec_buf, start, spec_len);
            spec_buf[spec_len] = '\0';

            // Use the type-specific reader
            TypeSpecificIO *io = argv++;
            argc--;
            if (!io->reader) {
                LOG_FATAL("Missing reader function");
            }

            char *read_head = NULL;
            if (remaining > 1) {
                read_head             = (char *)in;
                size        read_len  = strlen(in);
                const char *read_tail = read_head + read_len;
                while (read_head < read_tail) {
                    if (*read_head == p[1]) {
                        // null-terminate temporarily to keep read length constrained
                        *read_head = 0;
                        break;
                    }

                    read_head++;
                }

                if (read_head == read_tail) {
                    read_head = NULL;
                }
            }

            const char *next = io->reader(in, io->data);

            // if input was null-terminated then put back the original character
            if (read_head) {
                *read_head = p[1];
            }

            if (!next || next < in) {
                LOG_FATAL("Reader failed to advance input");
            }

            in = next;

            // Skip closing '}'
            p++;
            remaining--;
        } else {
            // Match exact character from format string
            if (!in || *in != *p) {
                LOG_ERROR(
                    "Input '%.*s' does not match format string '%.*s'",
                    MIN2(remaining, 8),
                    in,
                    MIN2(remaining, 8),
                    p
                );
                return NULL;
            }
            in++;
            p++;
            remaining--;
        }
    }

    return in;
}

#if defined(_WIN32)
#    include <io.h>
#    define ISATTY _isatty
#    define FILENO _fileno
#else
#    include <unistd.h>
#    define ISATTY isatty
#    define FILENO fileno
#endif

void FReadFmtInternal(FILE *file, const char *fmtstr, TypeSpecificIO *argv, size argc) {
    if (!file || !fmtstr) {
        LOG_FATAL("Invalid arguments");
    }

    Str buffer = StrInit();

    int    c;
    fpos_t start_pos;
    bool   can_rollback = false;

    // Try to check if the file is seekable
    int fd = FILENO(file);
    if (fd >= 0 && !ISATTY(fd)) {
        if (fgetpos(file, &start_pos) == 0) {
            can_rollback = true;
        } else {
            Str err = StrInit();
            LOG_ERROR("Could not save file position for rollback: %s", SysStrError(errno, &err)->data);
            StrDeinit(&err);
        }
    }

    while ((c = fgetc(file)) != EOF && c != '\n') {
        StrPushBack(&buffer, c);
    }

    if (buffer.length) {
        if (!StrReadFmtInternal(buffer.data, fmtstr, argv, argc)) {
            if (can_rollback) {
                fsetpos(file, &start_pos);
            } else {
                LOG_ERROR("Parse failed, and rollback not possible on non-seekable input");
            }
        }
    }

    StrDeinit(&buffer);
}

// Helper function to pad string with spaces
static void PadString(Str* o, size_t width, Alignment align, size_t content_len) {
    if (width <= content_len) return;
    
    size_t pad_len = width - content_len;
    Str temp = StrInit();
    
    switch (align) {
        case ALIGN_LEFT:
            // Original content followed by spaces
            for (size_t i = 0; i < pad_len; i++) {
                StrPushBack(o, ' ');
            }
            break;
            
        case ALIGN_RIGHT:
            // Spaces followed by original content
            for (size_t i = 0; i < pad_len; i++) {
                StrPushBack(&temp, ' ');
            }
            // Copy original content to temp
            for (size_t i = 0; i < content_len; i++) {
                StrPushBack(&temp, o->data[i]);
            }
            // Clear original and copy back
            StrClear(o);
            for (size_t i = 0; i < temp.length; i++) {
                StrPushBack(o, temp.data[i]);
            }
            break;
            
        case ALIGN_CENTER: {
            size_t left_pad = pad_len / 2;
            size_t right_pad = pad_len - left_pad;
            
            // Left padding
            for (size_t i = 0; i < left_pad; i++) {
                StrPushBack(&temp, ' ');
            }
            
            // Original content
            for (size_t i = 0; i < content_len; i++) {
                StrPushBack(&temp, o->data[i]);
            }
            
            // Right padding
            for (size_t i = 0; i < right_pad; i++) {
                StrPushBack(&temp, ' ');
            }
            
            // Copy back to original
            StrClear(o);
            for (size_t i = 0; i < temp.length; i++) {
                StrPushBack(o, temp.data[i]);
            }
            break;
        }
    }
    
    StrDeinit(&temp);
}

void _write_Str(Str *o, FmtInfo *fmt_info, Str *s) {
    if (!o || !fmt_info) {
        LOG_FATAL("Invalid arguments.");
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;
    
    // Handle null or empty string
    if (!s || !s->data) {
        StrAppendf(o, "(null)");
    } else {
        if (fmt_info->is_hex) {
            char *c = s->data;
            size  l = s->length;
            while (l) {
                if (l > 1) {
                    StrAppendf(o, fmt_info->is_caps ? "%.*X " : "%.*x ", (u32)fmt_info->width, (u32)*c);
                } else {
                    StrAppendf(o, fmt_info->is_caps ? "%.*X" : "%.*x", (u32)fmt_info->width, (u32)*c);
                }
                c++;
                l--;
            }
        } else {
            // If width is specified and less than string length, truncate
            size_t len = s->length;
            if (fmt_info->width > 0 && fmt_info->width < len) {
                len = fmt_info->width;
            }
            for (size_t i = 0; i < len; i++) {
                StrPushBack(o, s->data[i]);
            }
        }
    }

    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size_t content_len = o->length - start_len;
        PadString(o, fmt_info->width, fmt_info->align, content_len);
    }
}

void _write_Zstr(Str *o, FmtInfo *fmt_info, const char **s) {
    if (!o || !fmt_info) {
        LOG_FATAL("Invalid arguments.");
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;

    const char *str = *s;
    if (!str) {
        StrAppendf(o, "(null)");
    } else {
        if (fmt_info->is_hex) {
            while (*str) {
                if (*(str + 1)) {
                    StrAppendf(o, fmt_info->is_caps ? "%.*X " : "%.*x ", (u32)fmt_info->width, (u32)*str);
                } else {
                    StrAppendf(o, fmt_info->is_caps ? "%.*X" : "%.*x", (u32)fmt_info->width, (u32)*str);
                }
                str++;
            }
        } else {
            // If width is specified and less than string length, truncate
            size_t len = strlen(str);
            if (fmt_info->width > 0 && fmt_info->width < len) {
                len = fmt_info->width;
            }
            for (size_t i = 0; i < len; i++) {
                StrPushBack(o, str[i]);
            }
        }
    }

    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size_t content_len = o->length - start_len;
        PadString(o, fmt_info->width, fmt_info->align, content_len);
    }
}

void _write_u64(Str* o, FmtInfo* fmt_info, u64* v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;
    
    // Create temporary buffer for number formatting
    Str temp = StrInit();
    
    // Handle different bases
    if (fmt_info->is_hex) {
        // Add 0x prefix for hex
        StrPushBack(&temp, '0');
        StrPushBack(&temp, fmt_info->is_caps ? 'X' : 'x');
        
        // Convert to hex
        u64 val = *v;
        Str hex = StrInit();
        do {
            u8 digit = val & 0xF;
            char c = digit < 10 ? '0' + digit : (fmt_info->is_caps ? 'A' : 'a') + (digit - 10);
            StrPushBack(&hex, c);
            val >>= 4;
        } while (val);
        
        // Reverse the hex string
        for (size_t i = hex.length; i > 0; i--) {
            StrPushBack(&temp, hex.data[i - 1]);
        }
        StrDeinit(&hex);
        
    } else if (fmt_info->is_binary) {
        // Add 0b prefix for binary
        StrPushBack(&temp, '0');
        StrPushBack(&temp, 'b');
        
        // Convert to binary
        u64 val = *v;
        if (val == 0) {
            StrPushBack(&temp, '0');
        } else {
            Str bin = StrInit();
            while (val) {
                StrPushBack(&bin, '0' + (val & 1));
                val >>= 1;
            }
            
            // Reverse the binary string
            for (size_t i = bin.length; i > 0; i--) {
                StrPushBack(&temp, bin.data[i - 1]);
            }
            StrDeinit(&bin);
        }
    } else if (fmt_info->is_octal) {
        // Add 0o prefix for octal
        StrPushBack(&temp, '0');
        StrPushBack(&temp, 'o');
        
        // Convert to octal
        u64 val = *v;
        Str oct = StrInit();
        do {
            StrPushBack(&oct, '0' + (val & 7));
            val >>= 3;
        } while (val);
        
        // Reverse the octal string
        for (size_t i = oct.length; i > 0; i--) {
            StrPushBack(&temp, oct.data[i - 1]);
        }
        StrDeinit(&oct);
    } else {
        // Decimal format
        u64 val = *v;
        Str dec = StrInit();
        do {
            StrPushBack(&dec, '0' + (val % 10));
            val /= 10;
        } while (val);
        
        // Reverse the decimal string
        for (size_t i = dec.length; i > 0; i--) {
            StrPushBack(&temp, dec.data[i - 1]);
        }
        StrDeinit(&dec);
    }
    
    // Merge the formatted number into output
    StrMerge(o, &temp);
    StrDeinit(&temp);
    
    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size_t content_len = o->length - start_len;
        PadString(o, fmt_info->width, fmt_info->align, content_len);
    }
}

void _write_u32(Str* o, FmtInfo* fmt_info, u32* v) {
    // Convert to u64 and use the u64 writer
    u64 val = *v;
    _write_u64(o, fmt_info, &val);
}

void _write_u16(Str* o, FmtInfo* fmt_info, u16* v) {
    // Convert to u64 and use the u64 writer
    u64 val = *v;
    _write_u64(o, fmt_info, &val);
}

void _write_u8(Str* o, FmtInfo* fmt_info, u8* v) {
    // Convert to u64 and use the u64 writer
    u64 val = *v;
    _write_u64(o, fmt_info, &val);
}

void _write_i64(Str* o, FmtInfo* fmt_info, i64* v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;
    
    // Create temporary buffer for number formatting
    Str temp = StrInit();
    
    // Handle different bases
    if (fmt_info->is_hex) {
        // Add 0x prefix for hex (always lowercase)
        StrPushBack(&temp, '0');
        StrPushBack(&temp, 'x');
        
        // Convert to hex - use unsigned representation for hex
        // For negative numbers, we only want the lower 32 bits
        u64 val = (u64)(*v & 0xFFFFFFFF);  // Mask to 32 bits
        Str hex = StrInit();
        do {
            u8 digit = val & 0xF;
            char c = digit < 10 ? '0' + digit : (fmt_info->is_caps ? 'A' : 'a') + (digit - 10);
            StrPushBack(&hex, c);
            val >>= 4;
        } while (val);
        
        // Reverse the hex string
        for (size_t i = hex.length; i > 0; i--) {
            StrPushBack(&temp, hex.data[i - 1]);
        }
        StrDeinit(&hex);
        
    } else if (fmt_info->is_binary) {
        // Add 0b prefix for binary
        if (*v < 0) {
            StrPushBack(&temp, '-');
        }
        StrPushBack(&temp, '0');
        StrPushBack(&temp, 'b');
        
        // Convert to binary
        i64 val = *v < 0 ? -(*v) : *v;
        if (val == 0) {
            StrPushBack(&temp, '0');
        } else {
            Str bin = StrInit();
            while (val) {
                StrPushBack(&bin, '0' + (val & 1));
                val >>= 1;
            }
            
            // Reverse the binary string
            for (size_t i = bin.length; i > 0; i--) {
                StrPushBack(&temp, bin.data[i - 1]);
            }
            StrDeinit(&bin);
        }
    } else if (fmt_info->is_octal) {
        // Add 0o prefix for octal
        if (*v < 0) {
            StrPushBack(&temp, '-');
        }
        StrPushBack(&temp, '0');
        StrPushBack(&temp, 'o');
        
        // Convert to octal
        i64 val = *v < 0 ? -(*v) : *v;
        Str oct = StrInit();
        do {
            StrPushBack(&oct, '0' + (val & 7));
            val >>= 3;
        } while (val);
        
        // Reverse the octal string
        for (size_t i = oct.length; i > 0; i--) {
            StrPushBack(&temp, oct.data[i - 1]);
        }
        StrDeinit(&oct);
    } else {
        // Decimal format
        i64 val = *v;
        bool is_negative = val < 0;
        if (is_negative) {
            val = -val;
        }
        
        Str dec = StrInit();
        do {
            StrPushBack(&dec, '0' + (val % 10));
            val /= 10;
        } while (val);
        
        if (is_negative) {
            StrPushBack(&temp, '-');
        }
        
        // Reverse the decimal string
        for (size_t i = dec.length; i > 0; i--) {
            StrPushBack(&temp, dec.data[i - 1]);
        }
        StrDeinit(&dec);
    }
    
    // Merge the formatted number into output
    StrMerge(o, &temp);
    StrDeinit(&temp);
    
    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size_t content_len = o->length - start_len;
        PadString(o, fmt_info->width, fmt_info->align, content_len);
    }
}

void _write_i32(Str* o, FmtInfo* fmt_info, i32* v) {
    // Convert to i64 and use the i64 writer
    i64 val = *v;
    _write_i64(o, fmt_info, &val);
}

void _write_i16(Str* o, FmtInfo* fmt_info, i16* v) {
    // Convert to i64 and use the i64 writer
    i64 val = *v;
    _write_i64(o, fmt_info, &val);
}

void _write_i8(Str* o, FmtInfo* fmt_info, i8* v) {
    // Convert to i64 and use the i64 writer
    i64 val = *v;
    _write_i64(o, fmt_info, &val);
}

void _write_f64(Str* o, FmtInfo* fmt_info, f64* v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;
    
    // Create temporary buffer for number formatting
    Str temp = StrInit();
    
    // Handle special values first
    if (isnan(*v)) {
        StrAppendf(&temp, "nan");
    } else if (isinf(*v)) {
        StrAppendf(&temp, "inf");
    } else {
        // Handle scientific notation
        if (fmt_info->is_scientific || 
            (*v != 0 && (fabs(*v) < 0.0001 || fabs(*v) > 9999999.0))) {  // Auto scientific for very large/small
            // Get mantissa and exponent using frexp
            int bin_exp;
            f64 mantissa = frexp(*v, &bin_exp);
            
            // Convert binary exponent to decimal exponent
            // log10(2) ≈ 0.301, so multiply by that to get decimal exponent
            i32 dec_exp = (i32)(bin_exp * 0.301029995663981);
            
            // Adjust mantissa to be in [1.0, 10.0)
            mantissa = mantissa * pow(2.0, bin_exp - dec_exp * log2(10.0));
            
            // Handle edge cases where mantissa is slightly below 1.0
            if (mantissa < 1.0) {
                mantissa *= 10.0;
                dec_exp--;
            }
            
            // Format mantissa with precision
            size_t precision = fmt_info->has_precision ? fmt_info->precision : 5;
            if (*v < 0) StrPushBack(&temp, '-');
            
            // Format mantissa with precision and trim trailing zeros
            char buf[32];
            snprintf(buf, sizeof(buf), "%.*f", (u32)precision, mantissa);
            
            // Trim trailing zeros after decimal point
            char* decimal = strchr(buf, '.');
            if (decimal) {
                char* end = decimal + strlen(decimal) - 1;
                while (end > decimal && *end == '0') {
                    *end = '\0';
                    end--;
                }
                if (end == decimal) *end = '\0';  // Remove decimal point if no fractional part
            }
            
            StrAppendf(&temp, "%s", buf);
            
            // Add exponent
            StrPushBack(&temp, fmt_info->is_caps ? 'E' : 'e');
            StrPushBack(&temp, dec_exp < 0 ? '-' : '+');
            StrAppendf(&temp, "%d", abs(dec_exp));  // No leading zeros
        } else {
            // Regular decimal format
            size_t precision = fmt_info->has_precision ? fmt_info->precision : 11;
            
            // Format with precision and trim trailing zeros
            char buf[64];  // Increased buffer size for very large numbers
            snprintf(buf, sizeof(buf), "%.*f", (u32)precision, *v);
            
            // Trim trailing zeros after decimal point
            char* decimal = strchr(buf, '.');
            if (decimal) {
                char* end = decimal + strlen(decimal) - 1;
                while (end > decimal && *end == '0') {
                    *end = '\0';
                    end--;
                }
                if (end == decimal) *end = '\0';  // Remove decimal point if no fractional part
            }
            
            StrAppendf(&temp, "%s", buf);
        }
    }
    
    // Merge the formatted number into output
    StrMerge(o, &temp);
    StrDeinit(&temp);
    
    // Apply padding if width is specified
    if (fmt_info->width > 0) {
        size_t content_len = o->length - start_len;
        PadString(o, fmt_info->width, fmt_info->align, content_len);
    }
}

void _write_f32(Str* o, FmtInfo* fmt_info, f32* v) {
    // Convert to f64 and use the f64 writer
    f64 val = *v;
    _write_f64(o, fmt_info, &val);
}

const char *_read_Str(const char *i, Str *s) {
    if (!i || !s) {
        LOG_FATAL("Invalid arguments.");
    }

    const char *start = i;

    Str r = StrInit();
    while (*i && !isspace(*i)) {
        if (*i == '\\' && (i[1] == '"' || i[1] == '\'')) {
            StrPushBack(&r, i[0]);
            StrPushBack(&r, i[1]);
            i += 2;
        } else if (*i == '"' || *i == '\'') {
            char e = *i;
            i++;
            while (*i && *i != e) {
                StrPushBack(&r, *i);
                i++;
            }

            if (*i != e) {
                LOG_ERROR("Unexpected end of string input, expected '%c'", e);
                StrDeinit(&r);
                return start;
            }
        } else {
            StrPushBack(&r, *i);
        }

        i++;
    }
    *s = r;

    return i;
}

const char *_read_u8(const char *i, u8 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno             = 0;
    unsigned long val = strtoul(i, &end, 0);
    if (errno || end == i || val > UINT8_MAX)
        LOG_ERROR("Failed to parse u8");

    *v = (u8)val;
    return end;
}

const char *_read_u16(const char *i, u16 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno             = 0;
    unsigned long val = strtoul(i, &end, 0);
    if (errno || end == i || val > UINT16_MAX)
        LOG_ERROR("Failed to parse u16");

    *v = (u16)val;
    return end;
}

const char *_read_u32(const char *i, u32 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno             = 0;
    unsigned long val = strtoul(i, &end, 0);
    if (errno || end == i || val > UINT32_MAX)
        LOG_ERROR("Failed to parse u32");

    *v = (u32)val;
    return end;
}

const char *_read_u64(const char *i, u64 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno                  = 0;
    unsigned long long val = strtoull(i, &end, 0);
    if (errno || end == i)
        LOG_ERROR("Failed to parse u64");

    *v = (u64)val;
    return end;
}

const char *_read_i8(const char *i, i8 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno    = 0;
    long val = strtol(i, &end, 0);
    if (errno || end == i || val < INT8_MIN || val > INT8_MAX)
        LOG_ERROR("Failed to parse i8");

    *v = (i8)val;
    return end;
}

const char *_read_i16(const char *i, i16 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno    = 0;
    long val = strtol(i, &end, 0);
    if (errno || end == i || val < INT16_MIN || val > INT16_MAX)
        LOG_ERROR("Failed to parse i16");

    *v = (i16)val;
    return end;
}

const char *_read_i32(const char *i, i32 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno    = 0;
    long val = strtol(i, &end, 0);
    if (errno || end == i || val < INT32_MIN || val > INT32_MAX)
        LOG_ERROR("Failed to parse i32");

    *v = (i32)val;
    return end;
}

const char *_read_i64(const char *i, i64 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno         = 0;
    long long val = strtoll(i, &end, 0);
    if (errno || end == i)
        LOG_ERROR("Failed to parse i64");

    *v = (i64)val;
    return end;
}

const char *_read_f32(const char *i, f32 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno = 0;
    f32 val = strtof(i, &end);
    if (errno || end == i)
        LOG_ERROR("Failed to parse f32");

    *v = val;
    return end;
}

const char *_read_f64(const char *i, f64 *v) {
    if (!i || !v)
        LOG_FATAL("Invalid arguments");

    char *end;
    errno = 0;
    f64 val = strtod(i, &end);
    if (errno || end == i)
        LOG_ERROR("Failed to parse f64");

    *v = val;
    return end;
}

const char *_read_Zstr(const char *i, const char **out) {
    if (!i || !out) {
        LOG_FATAL("Invalid arguments.");
    }

    Str r = StrInit();

    const char *start = i;
    while (*i && !isspace(*i)) {
        if (*i == '\\' && (i[1] == '"' || i[1] == '\'')) {
            StrPushBack(&r, i[0]);
            StrPushBack(&r, i[1]);
            i += 1;
        } else if (*i == '"' || *i == '\'') {
            char e = *i;
            i++;
            while (*i && *i != e) {
                StrPushBack(&r, *i);
                i++;
            }

            if (*i != e) {
                LOG_ERROR("Unexpected end of string input, expected '%c'", e);
                StrDeinit(&r);
                return start;
            }
        } else {
            StrPushBack(&r, *i);
        }

        i++;
    }

    char *copy = malloc(r.length + 1);
    memcpy(copy, r.data, r.length);
    copy[r.length] = 0;

    *out = copy;
    StrDeinit(&r);
    return i;
}

void _write_UnsupportedType(Str *o, FmtInfo *fmt_info, const char **s) {
    (void)o;
    (void)fmt_info;
    (void)s;
    LOG_ERROR("Attempt to write unsupported type");
}

const char *_read_UnsupportedType(const char *i, const char **s) {
    (void)s;
    LOG_ERROR("Attempt to read unsupported type.");
    return i;
}

