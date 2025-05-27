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
    if (!spec || !fi) {
        LOG_FATAL("Invalid arguments to ParseFormatSpec");
        return false;
    }
    // Empty format specifier is allowed, but spec pointer must not be NULL
    
    // Initialize format info with defaults
    *fi = (FmtInfo){
        .align = ALIGN_RIGHT,
        .width = 0,
        .precision = 6,
        .has_precision = false,
        .is_hex = false,
        .is_binary = false,
        .is_octal = false,
        .is_debug = false,
        .is_scientific = false,
        .is_caps = false
    };
    
    size_t pos = 0;
    
    // Skip initial colon if present
    if (pos < len && spec[pos] == ':') {
        pos++;
        // After colon, we must have some format specifier
        if (pos >= len) {
            LOG_ERROR("Empty format specifier after colon");
            return false;
        }
    }
    
    // Parse alignment
    if (pos < len) {
        switch (spec[pos]) {
            case '<': fi->align = ALIGN_LEFT; pos++; break;
            case '>': fi->align = ALIGN_RIGHT; pos++; break;
            case '^': fi->align = ALIGN_CENTER; pos++; break;
            default: break;
        }
    }
    
    // Parse width
    if (pos < len && spec[pos] >= '0' && spec[pos] <= '9') {
        size_t width = 0;
        while (pos < len && spec[pos] >= '0' && spec[pos] <= '9') {
            width = width * 10 + (spec[pos] - '0');
            pos++;
        }
        if (width > 0) {
            fi->width = width;
        }
    }
    
    // Parse format type first (before precision)
    bool found_type = false;
    size_t type_pos = pos;
    while (type_pos < len) {
        if (spec[type_pos] == '.') break;  // Stop at precision
        switch (spec[type_pos]) {
            case 'x': fi->is_hex = true; found_type = true; break;
            case 'X': fi->is_hex = true; fi->is_caps = true; found_type = true; break;
            case 'b': fi->is_binary = true; found_type = true; break;
            case 'o': fi->is_octal = true; found_type = true; break;
            case 'e': fi->is_scientific = true; found_type = true; break;
            case 'E': fi->is_scientific = true; fi->is_caps = true; found_type = true; break;
            case '?': fi->is_debug = true; found_type = true; break;
            default: break;
            }
        if (found_type) {
            pos = type_pos + 1;
            break;
        }
        type_pos++;
    }
    
    // Parse precision
    if (pos < len && spec[pos] == '.') {
        pos++;
        if (pos >= len || spec[pos] < '0' || spec[pos] > '9') {
            return false; // Precision must be followed by digits
        }
        size_t precision = 0;
        while (pos < len && spec[pos] >= '0' && spec[pos] <= '9') {
            precision = precision * 10 + (spec[pos] - '0');
            pos++;
        }
        fi->has_precision = true;
        fi->precision = precision;
    }
    
    // Parse any remaining format type after precision
    if (!found_type) {
        while (pos < len) {
            switch (spec[pos]) {
                case 'x': fi->is_hex = true; break;
                case 'X': fi->is_hex = true; fi->is_caps = true; break;
                case 'b': fi->is_binary = true; break;
                case 'o': fi->is_octal = true; break;
                case 'e': fi->is_scientific = true; break;
                case 'E': fi->is_scientific = true; fi->is_caps = true; break;
                case '?': fi->is_debug = true; break;
                default: return false; // Invalid format type
            }
            pos++;
        }
    }
    
    return true;
}

// Helper function to pad string with spaces
static void PadString(Str* o, size_t width, Alignment align, size_t content_len) {
    if (content_len >= width) return;
    
    size_t pad_len = width - content_len;
    
    if (align == ALIGN_RIGHT) {
        // Pad on left
        for (size_t i = 0; i < pad_len; i++) {
            StrPushFront(o, ' ');
        }
    } else if (align == ALIGN_LEFT) {
        // Pad on right
        for (size_t i = 0; i < pad_len; i++) {
            StrPushBack(o, ' ');
        }
    } else { // ALIGN_CENTER
        size_t left_pad = pad_len / 2;
        size_t right_pad = pad_len - left_pad;
        
        // Pad on left
        for (size_t i = 0; i < left_pad; i++) {
            StrPushFront(o, ' ');
        }
        
        // Pad on right
        for (size_t i = 0; i < right_pad; i++) {
            StrPushBack(o, ' ');
        }
    }
}

bool StrWriteFmtInternal(Str* o, const char* fmt, TypeSpecificIO* args, size_t argc) {
    if (!o || !fmt) {
        LOG_FATAL("Invalid arguments");
        return false;
    }

    size_t arg_idx = 0;
    size_t fmt_len = 0;
    while (fmt[fmt_len]) fmt_len++;

    for (size_t i = 0; i < fmt_len; i++) {
        if (fmt[i] == '{') {
            // Check for escaped brace
            if (i + 1 < fmt_len && fmt[i + 1] == '{') {
                StrPushBack(o, '{');
                i++; // Skip next brace
                continue;
            }

            // Find closing brace
            size_t brace_start = i;
            size_t brace_end = i + 1;
            while (brace_end < fmt_len && fmt[brace_end] != '}') {
                brace_end++;
            }

            // Error if no closing brace found
            if (brace_end >= fmt_len) {
                LOG_ERROR("Unclosed format specifier");
                return false;
            }

            // Extract format specifier
            size_t spec_len = brace_end - brace_start - 1;

            // Parse format specifier
            FmtInfo fmt_info;
            if (spec_len == 0) {
                // Empty format specifier {} is allowed, initialize with defaults
                fmt_info = (FmtInfo){
                    .align = ALIGN_RIGHT,
                    .width = 0,
                    .precision = 6,
                    .has_precision = false,
                    .is_hex = false,
                    .is_binary = false,
                    .is_octal = false,
                    .is_debug = false,
                    .is_scientific = false,
                    .is_caps = false
                };
            } else if (!ParseFormatSpec(fmt + brace_start + 1, spec_len, &fmt_info)) {
                LOG_ERROR("Invalid format specifier");
                return false;
            }

            // Check if we have enough arguments
            if (arg_idx >= argc) {
                LOG_ERROR("Not enough arguments for format string");
                return false;
            }
            
            // Get current argument
            TypeSpecificIO* arg = &args[arg_idx++];
            if (!arg->writer || !arg->data) {
                LOG_ERROR("Invalid argument");
                return false;
            }
            
            // Write the formatted value
            arg->writer(o, &fmt_info, arg->data);
            
            // Skip to end of format specifier
            i = brace_end;
        } else if (fmt[i] == '}') {
            // Check for escaped brace
            if (i + 1 < fmt_len && fmt[i + 1] == '}') {
                StrPushBack(o, '}');
                i++; // Skip next brace
                continue;
            }
            LOG_ERROR("Unmatched closing brace");
            return false;
        } else {
            StrPushBack(o, fmt[i]);
        }
    }

    // Check if we used all arguments
    if (arg_idx < argc) {
        LOG_ERROR("Too many arguments for format string");
        return false;
        }

    return true;
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

void _write_Str(Str *o, FmtInfo *fmt_info, Str *s) {
    if (!o || !fmt_info) {
        LOG_FATAL("Invalid arguments");
        return;
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;
    
    // Handle null string
    if (!s) {
        StrPushBackZstr(o, "(null)");
    }
    // Handle null data pointer
    else if (!s->data) {
        StrPushBackZstr(o, "(null)");
    }
    // Handle empty string - don't need special handling, just apply padding if needed
    else if (s->length == 0) {
        // Empty string - no action needed, padding will be applied below
    }
    else {
        if (fmt_info->is_hex) {
            // Format each character as hex
            for (size_t i = 0; i < s->length; i++) {
                if (i > 0) {
                    StrPushBack(o, ' ');
                }
                // Create hex string for each character
                Str hex = StrInit();
                StrFromU64(&hex, (u8)s->data[i], 16, fmt_info->is_caps);
                // Ensure 2 digits with leading zero
                if (hex.length == 1) {
                    StrPushFront(&hex, '0');
                }
                StrPushBackZstr(o, "0x");
                StrMerge(o, &hex);
                StrDeinit(&hex);
            }
        } else {
            // If precision is specified, use it as max length
            size_t len = s->length;
            if (fmt_info->has_precision) {
                // Precision 0 means empty string (not an error)
                if (fmt_info->precision == 0) {
                    len = 0;
                } else {
                len = MIN2(len, fmt_info->precision);
            }
            }
            
            // Copy string content
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
        LOG_FATAL("Invalid arguments");
        return;
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;

    // Handle null or empty string
    if (!s || !*s) {
        StrPushBackZstr(o, "(null)");
    } else if ((*s)[0] == '\0') {
        // Empty string - don't need special handling, just apply padding if needed
    } else {
        if (fmt_info->is_hex) {
            // Format each character as hex
            const char* str = *s;
            size_t i = 0;
            while (str[i]) {
                if (i > 0) {
                    StrPushBack(o, ' ');
                }
                // Create hex string for each character
                Str hex = StrInit();
                StrFromU64(&hex, (u8)str[i], 16, fmt_info->is_caps);
                // Ensure 2 digits with leading zero
                if (hex.length == 1) {
                    StrPushFront(&hex, '0');
                }
                StrPushBackZstr(o, "0x");
                StrMerge(o, &hex);
                StrDeinit(&hex);
                i++;
            }
        } else {
            // Get string length
            size_t len = 0;
            const char* str = *s;
            while (str[len]) len++;
            
            // If precision is specified, use it as max length
            if (fmt_info->has_precision) {
                // Precision 0 means empty string (not an error)
                if (fmt_info->precision == 0) {
                    len = 0;
                } else {
                    len = MIN2(len, fmt_info->precision);
            }
            }
            
            // Copy string content
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
        return;
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;
    
    // Create temporary buffer for number formatting
    Str temp = StrInit();
    
    // Determine base based on format flags
    u8 base = 10;  // default is decimal
    if (fmt_info->is_hex) {
        base = 16;
    } else if (fmt_info->is_binary) {
        base = 2;
    } else if (fmt_info->is_octal) {
        base = 8;
    }
    
    // Use StrFromU64 directly with the appropriate base
    StrFromU64(&temp, *v, base, fmt_info->is_caps);
    
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
    u64 val = *v;
    _write_u64(o, fmt_info, &val);
}

void _write_u16(Str* o, FmtInfo* fmt_info, u16* v) {
    u64 val = *v;
    _write_u64(o, fmt_info, &val);
}

void _write_u8(Str* o, FmtInfo* fmt_info, u8* v) {
    u64 val = *v;
    _write_u64(o, fmt_info, &val);
}

void _write_i64(Str* o, FmtInfo* fmt_info, i64* v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return;
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;
    
    // Create temporary buffer for number formatting
    Str temp = StrInit();
    
    // Determine base based on format flags
    u8 base = 10;  // default is decimal
    if (fmt_info->is_hex) {
        base = 16;
    } else if (fmt_info->is_binary) {
        base = 2;
    } else if (fmt_info->is_octal) {
        base = 8;
    }
    
    // Use StrFromI64 directly with the appropriate base
    StrFromI64(&temp, *v, base, fmt_info->is_caps);
    
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
    i64 val = *v;
    _write_i64(o, fmt_info, &val);
}

void _write_i16(Str* o, FmtInfo* fmt_info, i16* v) {
    i64 val = *v;
    _write_i64(o, fmt_info, &val);
}

void _write_i8(Str* o, FmtInfo* fmt_info, i8* v) {
    i64 val = *v;
    _write_i64(o, fmt_info, &val);
}

void _write_f64(Str* o, FmtInfo* fmt_info, f64* v) {
    if (!o || !fmt_info || !v) {
        LOG_FATAL("Invalid arguments");
        return;
    }

    // Store original length to calculate content size later
    size_t start_len = o->length;
    
    // Create temporary buffer for number formatting
    Str temp = StrInit();
    
    // Format the number using StrFromF64, which already handles special cases
    // (NaN, inf, zeros, etc.)
    u8 precision = fmt_info->has_precision ? fmt_info->precision : 6;
    
    // Pass the scientific notation flag to StrFromF64
    StrFromF64(&temp, *v, precision, fmt_info->is_scientific, fmt_info->is_caps);
    
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

