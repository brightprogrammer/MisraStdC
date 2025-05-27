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
static bool ParseFormatSpec(const char* spec, size_t len, FmtInfo* fi) {
    // Reset format info
    SetMemory(fi, 0, sizeof(FmtInfo));
    
    // Early return if empty spec
    if (len == 0) return true;
    
    const char* p = spec;
    const char* end = spec + len;
    
    // Check for named/positional parameter
    if (isalpha(*p) || isdigit(*p)) {
        while (p < end && (isalnum(*p) || *p == '_')) p++;
    }
    
    // Skip colon for format spec
    bool has_colon = false;
    if (p < end && *p == ':') {
        has_colon = true;
        p++;
        
        // Skip whitespace after colon
        while (p < end && isspace(*p)) p++;
        
        // If we have a colon, we must have valid format specifiers after it
        if (p >= end) {
            LOG_ERROR("Empty format specifier after ':'");
            return false;
        }
        
        // If we have a colon, we must have at least one format specifier
        bool has_format_spec = false;
        const char* peek = p;
        
        // Check for alignment
        if (*peek == '<' || *peek == '>' || *peek == '^') {
            has_format_spec = true;
        }
        // Check for width
        else if (isdigit(*peek)) {
            has_format_spec = true;
        }
        // Check for precision
        else if (*peek == '.') {
            has_format_spec = true;
        }
        // Check for format type
        else {
            switch (*peek) {
                case 'x': case 'X':
                case 'b': case 'o':
                case '?':
                case 'e': case 'E':
                    has_format_spec = true;
                    break;
            }
        }
        
        if (!has_format_spec) {
            LOG_ERROR("No valid format specifiers after ':'");
            return false;
        }
    }
    
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
        if (p >= end || !isdigit(*p)) {
            LOG_ERROR("Invalid precision specification");
            return false;
        }
        fi->precision = 0;
        while (p < end && isdigit(*p)) {
            fi->precision = fi->precision * 10 + (*p - '0');
            p++;
        }
        fi->has_precision = true;
    }
    
    // Skip whitespace before format type
    while (p < end && isspace(*p)) p++;
    
    // Handle format type
    if (p < end) {
        switch (*p) {
            case 'x':
                fi->is_hex = true;
                fi->is_caps = false;
                p++;
                break;
            case 'X':
                fi->is_hex = true;
                fi->is_caps = true;
                p++;
                break;
            case 'b':
                fi->is_binary = true;
                p++;
                break;
            case 'o':
                fi->is_octal = true;
                p++;
                break;
            case '?':
                fi->is_debug = true;
                p++;
                break;
            case 'e':
                fi->is_scientific = true;
                fi->is_caps = false;
                p++;
                break;
            case 'E':
                fi->is_scientific = true;
                fi->is_caps = true;
                p++;
                break;
            default:
                if (has_colon) {  // Only error if we had a colon
                    LOG_ERROR("Invalid format specifier '%c'", *p);
                    return false;
                }
                break;  // No format type is valid if no colon
        }
    } else if (has_colon) {
        // If we had a colon but no format type, that's an error
        LOG_ERROR("Missing format type after ':'");
        return false;
    }
    
    // Skip trailing whitespace
    while (p < end && isspace(*p)) p++;
    
    // Ensure we consumed all characters
    if (p < end) {
        LOG_ERROR("Invalid characters in format specifier");
        return false;
    }
    
    return true;
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
            if (!ParseFormatSpec(spec_buf, spec_len, &fi)) {
                LOG_ERROR("Invalid format specifier");
                return;
            }

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

    const char *p = fmtstr;
    const char *in = input;
    size remaining = strlen(fmtstr);
    size arg_index = 0;  // Current argument index

    while (remaining > 0) {
        if (remaining >= 2 && p[0] == '{' && p[1] == '{') {
            if (!in || *in != '{') {
                LOG_ERROR("Expected '{' in input");
                return NULL;
            }
            in++;
            p += 2;
            remaining -= 2;
        } else if (remaining >= 2 && p[0] == '}' && p[1] == '}') {
            if (!in || *in != '}') {
                LOG_ERROR("Expected '}' in input");
                return NULL;
            }
            in++;
            p += 2;
            remaining -= 2;
        } else if (p[0] == '{') {
            p++;
            remaining--;

            // Find closing brace
            const char* start = p;
            size spec_len = 0;
            while (remaining > 0 && *p != '}') {
                p++;
                remaining--;
                spec_len++;
            }

            if (remaining == 0 || *p != '}') {
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

            memcpy(spec_buf, start, spec_len);
            spec_buf[spec_len] = '\0';

            // Validate format specifier
            FmtInfo fmt_info;
            if (!ParseFormatSpec(spec_buf, spec_len, &fmt_info)) {
                return NULL;  // Error already logged by ParseFormatSpec
            }

            // Skip whitespace before value
            while (*in && isspace(*in)) in++;

            // Use the type-specific reader
            TypeSpecificIO *io = &argv[arg_index++];
            if (!io->reader) {
                LOG_ERROR("Missing reader function");
                return NULL;
            }

            const char* next = io->reader(in, io->data);

            // Check if reading failed
            if (!next || next == in) {
                LOG_ERROR("Failed to read value for placeholder %zu", arg_index - 1);
                return NULL;
            }

            // Update input pointer
            in = next;

            // Skip whitespace after value
            while (*in && isspace(*in)) in++;

            // Skip closing '}'
            p++;
            remaining--;
        } else {
            // Skip whitespace in format string if it's not part of a literal
            if (isspace(*p)) {
                // Skip whitespace in both format string and input
                while (remaining > 0 && isspace(*p)) {
                    p++;
                    remaining--;
                }
                while (*in && isspace(*in)) in++;
                continue;
            }

            // Match exact character from format string
            if (!in || *in != *p) {
                LOG_ERROR(
                    "Input '%.*s' does not match format string '%.*s'",
                    MIN2(remaining, 8),
                    in ? in : "(null)",
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

    // Skip any trailing whitespace in input
    while (*in && isspace(*in)) in++;

    // Check for extra input
    if (*in) {
        LOG_ERROR("Extra input after format string: '%.*s'", MIN2(8, strlen(in)), in);
        return NULL;
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

// Helper function to handle escape sequences
static char ProcessEscape(const char** str) {
    if (!str || !*str) return 0;
    
    const char* s = *str;
    if (*s != '\\') {
        LOG_ERROR("ProcessEscape called on non-escape sequence");
        return 0;
    }
    
    s++;  // Skip backslash
    char result = 0;
    
    switch (*s) {
        case 'n': result = '\n'; break;
        case 'r': result = '\r'; break;
        case 't': result = '\t'; break;
        case 'b': result = '\b'; break;
        case 'f': result = '\f'; break;
        case 'v': result = '\v'; break;
        case 'a': result = '\a'; break;
        case '\\': result = '\\'; break;
        case '"': result = '"'; break;
        case '\'': result = '\''; break;
        case '0': result = '\0'; break;
        case 'x': {  // Hex escape
            s++;
            if (!isxdigit(s[0]) || !isxdigit(s[1])) {
                LOG_ERROR("Invalid hex escape sequence");
                return 0;
            }
            char hex[3] = {s[0], s[1], '\0'};
            result = (char)strtol(hex, NULL, 16);
            s++;  // Point to second hex digit
            break;
        }
        default:
            LOG_ERROR("Invalid escape sequence '\\%c'", *s);
            return 0;
    }
    
    *str = s;  // Update pointer to last processed character
    return result;
}

const char *_read_Str(const char *i, Str *s) {
    if (!i || !s) LOG_FATAL("Invalid arguments");
    
    // Skip leading whitespace
    while (isspace(*i)) i++;
    
    // Check for empty input
    if (!*i) {
        LOG_ERROR("Empty input string");
        return i;
    }
    
    // Initialize output string
    *s = StrInit();
    
    // Check for quoted string
    char quote = 0;
    if (*i == '"' || *i == '\'') {
        quote = *i++;
    }
    
    while (*i) {
        if (quote) {
            // Quoted string mode
            if (*i == '\\') {
                const char* curr = i;
                char c = ProcessEscape(&curr);
                if (c == 0) {  // Error in escape sequence
                    StrDeinit(s);
                    return NULL;
                }
                i = curr + 1;  // Move past the escape sequence
                StrPushBack(s, c);
            } else if (*i == quote) {
                i++;  // Skip closing quote
                return i;  // Successfully read quoted string
            } else {
                StrPushBack(s, *i++);
            }
        } else {
            // Unquoted string mode - read until whitespace
            if (isspace(*i)) {
                return i;  // Successfully read unquoted string
            }
            
            if (*i == '\\') {
                const char* curr = i;
                char c = ProcessEscape(&curr);
                if (c == 0) {  // Error in escape sequence
                    StrDeinit(s);
                    return NULL;
                }
                i = curr + 1;  // Move past the escape sequence
                StrPushBack(s, c);
            } else {
                StrPushBack(s, *i++);
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

// Helper function to validate integer string
static bool IsValidIntegerString(const char* str) {
    if (!str) return false;
    
    // Skip whitespace
    while (isspace(*str)) str++;
    
    // Handle sign
    if (*str == '-' || *str == '+') str++;
    
    // Check for empty string after sign
    if (!*str) return false;
    
    // Check for hex prefix
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        if (!*str) return false;  // No digits after prefix
        size_t len = strspn(str, "0123456789abcdefABCDEF");
        if (len == 0) return false;  // No valid digits
        str += len;
        while (isspace(*str)) str++;  // Skip trailing whitespace
        return *str == '\0';  // Must be end of string
    }
    
    // Check for binary prefix
    if (str[0] == '0' && (str[1] == 'b' || str[1] == 'B')) {
        str += 2;
        if (!*str) return false;  // No digits after prefix
        size_t len = strspn(str, "01");
        if (len == 0) return false;  // No valid digits
        str += len;
        while (isspace(*str)) str++;  // Skip trailing whitespace
        return *str == '\0';  // Must be end of string
    }
    
    // Check for octal prefix
    if (str[0] == '0' && (str[1] == 'o' || str[1] == 'O')) {
        str += 2;
        if (!*str) return false;  // No digits after prefix
        size_t len = strspn(str, "01234567");
        if (len == 0) return false;  // No valid digits
        str += len;
        while (isspace(*str)) str++;  // Skip trailing whitespace
        return *str == '\0';  // Must be end of string
    }
    
    // Check for hex without prefix (must contain at least one hex letter)
    size_t hex_len = strspn(str, "0123456789abcdefABCDEF");
    if (hex_len > 0 && strspn(str, "abcdefABCDEF") > 0) {
        str += hex_len;
        while (isspace(*str)) str++;  // Skip trailing whitespace
        return *str == '\0';  // Must be end of string
    }
    
    // Check for decimal
    if (!isdigit(*str) && *str != '0') return false;  // Must start with a digit
    size_t dec_len = strspn(str, "0123456789");
    if (dec_len == 0) return false;  // No valid digits
    str += dec_len;
    
    // Check for invalid characters after the number
    while (isspace(*str)) str++;  // Skip trailing whitespace
    return *str == '\0';  // Must be end of string
}

// Helper function to determine the base from a string prefix
static int GetBase(const char** str) {
    if (!str || !*str) return 10;  // Default to decimal
    
    const char* s = *str;
    const char* orig = s;  // Save original position
    
    // Handle sign for base detection
    bool negative = (*s == '-');
    if (negative) s++;
    
    if (s[0] == '0') {
        if (s[1] == 'x' || s[1] == 'X') {
            *str = negative ? orig : s + 2;  // Skip prefix if no minus
            return 16;  // Hex
        } else if (s[1] == 'b' || s[1] == 'B') {
            *str = negative ? orig : s + 2;  // Skip prefix if no minus
            return 2;   // Binary
        } else if (s[1] == 'o' || s[1] == 'O') {
            *str = negative ? orig : s + 2;  // Skip prefix if no minus
            return 8;   // Octal
        }
    }
    
    // Try to detect hex without prefix
    size_t len = strlen(s);
    size_t hex_len = strspn(s, "0123456789abcdefABCDEF");
    if (hex_len == len && strspn(s, "abcdefABCDEF") > 0) {
        *str = negative ? orig : s;  // Don't skip anything
        return 16;  // Hex
    }
    
    // Try to detect binary without prefix
    if (strspn(s, "01") == len) {
        *str = negative ? orig : s;  // Don't skip anything
        return 2;  // Binary
    }
    
    *str = orig;  // Reset to original position
    return 10;  // Default to decimal
}

// Helper function to check for special floating point values
static bool ParseSpecialFloat(const char* str, f64* result) {
    if (!str || !result) return false;
    
    // Skip whitespace
    while (isspace(*str)) str++;
    
    // Check for special values
    bool negative = false;
    const char* parse_str = str;
    if (*parse_str == '-') {
        negative = true;
        parse_str++;
    } else if (*parse_str == '+') {
        parse_str++;
    }
    
    if (strncasecmp(parse_str, "inf", 3) == 0) {
        // Check that there are no extra characters
        const char* end = parse_str + 3;
        while (*end && isspace(*end)) end++;
        if (*end) return false;
        
        *result = negative ? -INFINITY : INFINITY;
        return true;
    } else if (strncasecmp(parse_str, "nan", 3) == 0) {
        // Check that there are no extra characters
        const char* end = parse_str + 3;
        while (*end && isspace(*end)) end++;
        if (*end) return false;
        
        *result = NAN;
        return true;
    }
    
    return false;
}

// Helper function to validate float string
static bool IsValidFloatString(const char* str, bool* is_special) {
    if (!str || !is_special) return false;
    *is_special = false;
    
    // Skip whitespace
    while (isspace(*str)) str++;
    
    // Handle sign
    if (*str == '-' || *str == '+') str++;
    
    // Check for special values
    const char* p = str;
    if (strncasecmp(p, "inf", 3) == 0) {
        p += 3;
        while (isspace(*p)) p++;
        *is_special = true;
        return *p == '\0';
    }
    if (strncasecmp(p, "nan", 3) == 0) {
        p += 3;
        while (isspace(*p)) p++;
        *is_special = true;
        return *p == '\0';
    }
    
    // Handle integer part
    size_t int_len = strspn(str, "0123456789");
    str += int_len;
    
    // Handle decimal point and fractional part
    if (*str == '.') {
        str++;
        size_t frac_len = strspn(str, "0123456789");
        str += frac_len;
        
        // Must have at least one digit in either integer or fractional part
        if (int_len == 0 && frac_len == 0) return false;
    } else if (int_len == 0) {
        return false;  // No digits before decimal point
    }
    
    // Handle scientific notation
    if (*str == 'e' || *str == 'E') {
        str++;
        if (*str == '+' || *str == '-') str++;
        size_t exp_len = strspn(str, "0123456789");
        if (exp_len == 0) return false;  // No exponent digits
        str += exp_len;
    }
    
    // Check for invalid characters after the number
    while (isspace(*str)) str++;  // Skip trailing whitespace
    return *str == '\0';  // Must be end of string
}

const char *_read_f64(const char *i, f64 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    // Skip whitespace
    while (isspace(*i)) i++;
    
    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse f64: empty input");
        return i;
    }
    
    // Find the end of the potential number
    const char* start = i;
    while (*i && !isspace(*i)) i++;
    
    // Create a temporary buffer for the number
    size_t len = i - start;
    char* temp = malloc(len + 1);
    if (!temp) {
        LOG_ERROR("Failed to allocate memory");
        return start;
    }
    memcpy(temp, start, len);
    temp[len] = '\0';
    
    bool is_special;
    if (!IsValidFloatString(temp, &is_special)) {
        LOG_ERROR("Failed to parse f64: invalid format");
        free(temp);
        return NULL;  // Return NULL to indicate failure
    }
    
    if (is_special) {
        // Try parsing special values
        if (ParseSpecialFloat(temp, v)) {
            free(temp);
            return i;  // Return position after the special value
        }
        free(temp);
        return NULL;  // Should not happen if IsValidFloatString passed
    }
    
    // Parse regular float
    char* end;
    errno = 0;
    *v = strtod(temp, &end);
    
    // Check for errors
    if (errno == ERANGE) {
        if (isinf(*v)) {
            LOG_ERROR("Failed to parse f64: value out of range");
            free(temp);
            return NULL;
        }
        if (*v == 0.0) {
            // Check if this is actually a zero value or an underflow
            const char* p = temp;
            while (isspace(*p)) p++;  // Skip whitespace
            if (*p == '-' || *p == '+') p++;  // Skip sign
            if (*p == '0') {
                // This is an actual zero
                free(temp);
                return i;
            }
            LOG_ERROR("Failed to parse f64: value too small");
            free(temp);
            return NULL;
        }
    }
    
    free(temp);
    return i;  // Return position after the number
}

// Helper function to parse unsigned integers with overflow checking
static bool ParseUnsigned(const char* str, u64* result, int base, u64 max_val) {
    if (!str || !result) return false;
    
    // Skip whitespace
    while (isspace(*str)) str++;
    
    // Check for empty string
    if (!*str) return false;
    
    // Check for negative numbers
    if (*str == '-') return false;
    
    // Validate input string
    if (!IsValidIntegerString(str)) return false;
    
    // Get base from prefix
    const char* parse_str = str;
    base = GetBase(&parse_str);
    
    // Parse number
    char* end;
    errno = 0;
    *result = strtoull(str, &end, base);
    
    // Check for errors
    if (errno == ERANGE || *result > max_val) return false;
    if (end == str) return false;  // No digits parsed
    if (*end && !isspace(*end)) return false;  // Invalid characters
    
    return true;
}

// Helper function to parse signed integers with overflow checking
static bool ParseSigned(const char* str, i64* result, int base, i64 min_val, i64 max_val) {
    if (!str || !result) return false;
    
    // Skip whitespace
    while (isspace(*str)) str++;
    
    // Check for empty string
    if (!*str) return false;
    
    // Validate input string
    if (!IsValidIntegerString(str)) return false;
    
    // Handle sign
    bool negative = (*str == '-');
    if (negative || *str == '+') str++;
    
    // Get base from prefix
    base = GetBase(&str);
    
    // Parse number
    char* end;
    errno = 0;
    i64 val = strtoll(str, &end, base);
    
    // Check for errors
    if (errno == ERANGE || val < min_val || val > max_val) return false;
    if (end == str) return false;  // No digits parsed
    if (*end && !isspace(*end)) return false;  // Invalid characters
    
    *result = val;
    return true;
}

const char *_read_u8(const char *i, u8 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    u64 val;
    if (!ParseUnsigned(i, &val, 10, UINT8_MAX)) {
        LOG_ERROR("Failed to parse u8");
        return i;
    }
    
    *v = (u8)val;
    // Skip the parsed number
    while (*i && !isspace(*i)) i++;
    return i;
}

const char *_read_u16(const char *i, u16 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    u64 val;
    if (!ParseUnsigned(i, &val, 10, UINT16_MAX)) {
        LOG_ERROR("Failed to parse u16");
        return i;
    }
    
    *v = (u16)val;
    // Skip the parsed number
    while (*i && !isspace(*i)) i++;
    return i;
}

const char *_read_u32(const char *i, u32 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    u64 val;
    if (!ParseUnsigned(i, &val, 10, UINT32_MAX)) {
        LOG_ERROR("Failed to parse u32");
        return i;
    }
    
    *v = (u32)val;
    // Skip the parsed number
    while (*i && !isspace(*i)) i++;
    return i;
}

const char *_read_u64(const char *i, u64 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    u64 val;
    if (!ParseUnsigned(i, &val, 10, UINT64_MAX)) {
        LOG_ERROR("Failed to parse u64");
        return i;
    }
    
    *v = val;
    // Skip the parsed number
    while (*i && !isspace(*i)) i++;
    return i;
}

const char *_read_i8(const char *i, i8 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    i64 val;
    if (!ParseSigned(i, &val, 10, INT8_MIN, INT8_MAX)) {
        LOG_ERROR("Failed to parse i8");
        return i;
    }
    
    *v = (i8)val;
    // Skip the parsed number
    while (*i && !isspace(*i)) i++;
    return i;
}

const char *_read_i16(const char *i, i16 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    i64 val;
    if (!ParseSigned(i, &val, 10, INT16_MIN, INT16_MAX)) {
        LOG_ERROR("Failed to parse i16");
        return i;
    }
    
    *v = (i16)val;
    // Skip the parsed number
    while (*i && !isspace(*i)) i++;
    return i;
}

const char *_read_i32(const char *i, i32 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    // Skip whitespace
    while (isspace(*i)) i++;
    
    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse i32: empty input");
        return i;
    }
    
    // Find the end of the potential number
    const char* start = i;
    while (*i && !isspace(*i)) i++;
    
    // Create a temporary buffer for the number
    size_t len = i - start;
    char* temp = malloc(len + 1);
    if (!temp) {
        LOG_ERROR("Failed to allocate memory");
        return start;
    }
    memcpy(temp, start, len);
    temp[len] = '\0';
    
    // Validate the number string
    if (!IsValidIntegerString(temp)) {
        LOG_ERROR("Failed to parse i32: invalid format");
        free(temp);
        return NULL;  // Return NULL to indicate failure
    }
    
    // Parse the number
    i64 val;
    if (!ParseSigned(temp, &val, 10, INT32_MIN, INT32_MAX)) {
        LOG_ERROR("Failed to parse i32: value out of range");
        free(temp);
        return NULL;  // Return NULL to indicate failure
    }
    
    free(temp);
    *v = (i32)val;
    return i;  // Return position after the number
}

const char *_read_i64(const char *i, i64 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    i64 val;
    if (!ParseSigned(i, &val, 10, INT64_MIN, INT64_MAX)) {
        LOG_ERROR("Failed to parse i64");
        return i;
    }
    
    *v = val;
    // Skip the parsed number
    while (*i && !isspace(*i)) i++;
    return i;
}

const char *_read_f32(const char *i, f32 *v) {
    if (!i || !v) LOG_FATAL("Invalid arguments");
    
    // Skip whitespace
    while (isspace(*i)) i++;
    
    // Check for empty string
    if (!*i) {
        LOG_ERROR("Failed to parse f32: empty input");
        return i;
    }
    
    bool is_special;
    if (!IsValidFloatString(i, &is_special)) {
        LOG_ERROR("Failed to parse f32: invalid format");
        return i;
    }
    
    if (is_special) {
        // Try parsing special values
        f64 special_val;
        if (ParseSpecialFloat(i, &special_val)) {
            *v = (f32)special_val;
            // Skip the parsed value
            while (*i && !isspace(*i)) i++;
            return i;
        }
        return i;  // Should not happen if IsValidFloatString passed
    }
    
    // Parse regular float
    char* end;
    errno = 0;
    f32 val = strtof(i, &end);
    
    // Check for errors
    if (errno == ERANGE) {
        if (isinf(val)) {
            LOG_ERROR("Failed to parse f32: value out of range");
            return i;
        }
        if (val == 0.0f) {
            // Check if this is actually a zero value or an underflow
            const char* p = i;
            while (isspace(*p)) p++;  // Skip whitespace
            if (*p == '-' || *p == '+') p++;  // Skip sign
            if (*p == '0') {
                // This is an actual zero
                *v = val;
                return end;
            }
            LOG_ERROR("Failed to parse f32: value too small");
            return i;
        }
    }
    
    *v = val;
    return end;
}

const char *_read_Zstr(const char *i, const char **out) {
    if (!i || !out) LOG_FATAL("Invalid arguments");
    
    // Use Str to read the string
    Str temp = StrInit();
    const char* next = _read_Str(i, &temp);
    
    // Check if reading failed
    if (next == i) {
        StrDeinit(&temp);
        return i;
    }
    
    // Allocate and copy to null-terminated string
    char* result = malloc(temp.length + 1);
    if (!result) {
        LOG_ERROR("Failed to allocate memory for string");
        StrDeinit(&temp);
        return i;
    }
    
    memcpy(result, temp.data, temp.length);
    result[temp.length] = '\0';
    
    *out = result;
    StrDeinit(&temp);
    return next;
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

