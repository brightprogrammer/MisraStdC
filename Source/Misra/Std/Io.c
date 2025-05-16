/// file      : std/io.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, All rights reserved.
///
/// formatted reading/writing and other magical stuff

#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>

// stdc
#include <ctype.h>
#include <stdarg.h>

void _write_Str(Str *o, FmtInfo *fmt_info, Str *s) {
    if (!o || !s) {
        LOG_FATAL("Invalid arguments.");
    }

    if (s->data && s->length) {
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
            StrAppendf(o, "%s", s->data);
        }
    } else {
        StrAppendf(o, "(null)");
    }
};

void _write_u8(Str *o, FmtInfo *fmt_info, u8 *v) {
    if (!o || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    fmt_info->width = CLAMP(fmt_info->width, 2, 8);
    if (fmt_info->is_hex) {
        StrAppendf(o, fmt_info->is_caps ? "%.*X" : "%.*x", fmt_info->width, (u32)*v);
    } else {
        StrAppendf(o, "%u", (u32)*v);
    }
}

void _write_u16(Str *o, FmtInfo *fmt_info, u16 *v) {
    if (!o || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    int width = CLAMP(fmt_info->width, 4, 8);
    if (fmt_info->is_hex) {
        StrAppendf(o, fmt_info->is_caps ? "%.*X" : "%.*x", width, (u32)*v);
    } else {
        StrAppendf(o, "%u", (u32)*v);
    }
}

void _write_u32(Str *o, FmtInfo *fmt_info, u32 *v) {
    if (!o || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    int width = CLAMP(fmt_info->width, 8, 8);
    if (fmt_info->is_hex) {
        StrAppendf(o, fmt_info->is_caps ? "%.*X" : "%.*x", width, *v);
    } else {
        StrAppendf(o, "%u", *v);
    }
}

void _write_u64(Str *o, FmtInfo *fmt_info, u64 *v) {
    if (!o || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    int width = CLAMP(fmt_info->width, 8, 16);
    if (fmt_info->is_hex) {
        StrAppendf(o, fmt_info->is_caps ? "%.*llX" : "%.*llx", width, (unsigned long long)*v);
    } else {
        StrAppendf(o, "%llu", (unsigned long long)*v);
    }
}

void _write_i8(Str *o, FmtInfo *fmt_info, i8 *v) {
    if (!o || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    int width = CLAMP(fmt_info->width, 2, 8);
    if (fmt_info->is_hex) {
        StrAppendf(o, fmt_info->is_caps ? "%.*X" : "%.*x", width, (u32)(*v & 0xFF));
    } else {
        StrAppendf(o, "%d", (int)*v);
    }
}

void _write_i16(Str *o, FmtInfo *fmt_info, i16 *v) {
    if (!o || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    int width = CLAMP(fmt_info->width, 4, 8);
    if (fmt_info->is_hex) {
        StrAppendf(o, fmt_info->is_caps ? "%.*X" : "%.*x", width, (u32)(*v & 0xFFFF));
    } else {
        StrAppendf(o, "%d", (int)*v);
    }
}

void _write_i32(Str *o, FmtInfo *fmt_info, i32 *v) {
    if (!o || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    int width = CLAMP(fmt_info->width, 8, 8);
    if (fmt_info->is_hex) {
        StrAppendf(o, fmt_info->is_caps ? "%.*X" : "%.*x", width, (u32)(*v));
    } else {
        StrAppendf(o, "%d", *v);
    }
}

void _write_i64(Str *o, FmtInfo *fmt_info, i64 *v) {
    if (!o || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    int width = CLAMP(fmt_info->width, 8, 16);
    if (fmt_info->is_hex) {
        StrAppendf(o, fmt_info->is_caps ? "%.*llX" : "%.*llx", width, (u64)(*v));
    } else {
        StrAppendf(o, "%lld", (long long)*v);
    }
}


const char *_read_Str(const char *i, Str *s) {
    if (!i || !s) {
        LOG_FATAL("Invalid arguments.");
    }

    Str r = StrInit();
    while (*i && !isspace(*i)) {
        StrPushBack(&r, *i);
        i++;
    }
    *s = r;

    return i;
};

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

void StrWriteFmt(Str *o, const char *fmtstr, ...) {
    if (!o || !fmtstr) {
        LOG_FATAL("Invalid arguments");
    }

    va_list args;
    va_start(args, fmtstr);

    const char *p         = fmtstr;
    size_t      remaining = strlen(fmtstr);

    while (remaining > 0) {
        if (remaining >= 2 && p[0] == '{' && p[1] == '{') {
            StrPushBack(o, '{');
            p         += 2;
            remaining -= 2;
        } else if (remaining >= 2 && p[0] == '}' && p[1] == '}') {
            StrPushBack(o, '}');
            p         += 2;
            remaining -= 2;
        } else if (p[0] == '{') {
            p++;
            remaining--;

            const char *start    = p;
            size_t      spec_len = 0;

            while (remaining > 0 && *p != '}') {
                p++;
                remaining--;
                spec_len++;
            }

            if (remaining == 0 || *p != '}') {
                LOG_FATAL("Unmatched '{' in format string");
            }

            // Parse the specifier into a null-terminated string
            char spec_buf[32] = {0}; // 31 chars max + NUL
            if (spec_len >= sizeof(spec_buf)) {
                LOG_ERROR("Format specifier too long");
            }

            memcpy(spec_buf, start, spec_len);
            spec_buf[spec_len] = '\0';

            // Extract format info
            FmtInfo     fi = {0};
            const char *s  = spec_buf;

            if (*s == '#') {
                s++;
                if (*s == 'x') {
                    fi.is_hex  = true;
                    fi.is_caps = false;
                    s++;
                } else if (*s == 'X') {
                    fi.is_hex  = true;
                    fi.is_caps = true;
                    s++;
                }
            }

            if (*s >= '0' && *s <= '9') {
                fi.width = (int)strtol(s, NULL, 10);
            }

            // Consume closing '}'
            p++;
            remaining--;

            // Write argument using type-specific writer
            TypeSpecificIO io = va_arg(args, TypeSpecificIO);
            if (!io.writer) {
                LOG_FATAL("Missing writer function");
            }
            io.writer(o, &fi, io.data);
        } else {
            StrPushBack(o, *p);
            p++;
            remaining--;
        }
    }

    va_end(args);
}

const char *StrReadFmt(const char *input, const char *fmtstr, ...) {
    if (!input || !fmtstr) {
        LOG_FATAL("Invalid arguments");
    }

    const char *p         = fmtstr;
    size_t      remaining = strlen(fmtstr);
    const char *in        = input;

    va_list args;
    va_start(args, fmtstr);

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
            size_t      spec_len = 0;

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
            memcpy(spec_buf, start, spec_len);
            spec_buf[spec_len] = '\0';

            // Use the type-specific reader
            TypeSpecificIO io = va_arg(args, TypeSpecificIO);
            if (!io.reader) {
                LOG_FATAL("Missing reader function");
            }

            const char *next = io.reader(in, io.data);
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
                LOG_FATAL("Input does not match format string");
            }
            in++;
            p++;
            remaining--;
        }
    }

    va_end(args);
    return in;
}
