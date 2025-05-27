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

void StrWriteFmtInternal(Str *o, const char *fmtstr, TypeSpecificIO *argv, size argc) {
    if (!o || !fmtstr) {
        LOG_FATAL("Invalid arguments");
    }

    const char *p         = fmtstr;
    size        remaining = strlen(fmtstr);

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
            size        spec_len = 0;

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
                LOG_FATAL("Format specifier too long");
            }

            if (!argc) {
                LOG_FATAL("More placeholders than arguments.");
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
            TypeSpecificIO *io = argv++;
            argc--;
            if (!io->writer) {
                LOG_FATAL("Missing writer function");
            }
            io->writer(o, &fi, io->data);
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
}

void _write_Zstr(Str *o, FmtInfo *fmt_info, const char **s) {
    if (!o || !s) {
        LOG_FATAL("Invalid arguments.");
    }

    const char *c = *s;
    size        l = strlen(c);
    if (c && l) {
        if (fmt_info->is_hex) {
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
            StrAppendf(o, "%s", c);
        }
    } else {
        StrAppendf(o, "(null)");
    }
}

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

void _write_UnsupportedType(Str *o, FmtInfo *fmt_info, const char **s) {
    (void)o;
    (void)fmt_info;
    (void)s;
    LOG_ERROR("Attempt to write unsupported type");
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
    return i;
}

const char *_read_UnsupportedType(const char *i, const char **s) {
    (void)s;
    LOG_ERROR("Attempt to read unsupported type.");
    return i;
}
